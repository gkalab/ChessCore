//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// PlayGames.cpp: Play game(s) between two engines.
//

#include "ccore.h"
#include <ChessCore/Database.h>
#include <ChessCore/PgnDatabase.h>
#include <ChessCore/Engine.h>
#include <ChessCore/TimeControl.h>
#include <ChessCore/Log.h>
#include <ChessCore/IoEventWaiter.h>
#include <ChessCore/OpeningTree.h>
#include <iomanip>
#include <memory>
#include <iostream>
#include <sstream>
#include <errno.h>

using namespace std;
using namespace ChessCore;

static const char *m_classname = "PlayGames";

// Maximum we will wait before getting bored
const int ENGINE_WAIT_TIMEOUT = 180 * 1000;

// Tournament data
struct Tournament {
    unsigned numRounds;
    unsigned gameNum;
    Engine *white;
    Engine *black;
    Game game;

    Tournament() :
        numRounds(0),
        gameNum(0),
        white(0),
        black(0),
        game() {
    }
};

struct EngineScore {
    unsigned wins;
    unsigned loses;
    unsigned draws;

    EngineScore() :
        wins(0),
        loses(0),
        draws(0) {
    }
};

static bool playGame(Tournament &tourney, string &gameOverReason);
static bool displayInfo(const shared_ptr<EngineMessage> &message, int &score, int &mateScore);
static void engineUciDebug(void *userp, const Engine *engine, bool fromEngine, const std::string &message);
static bool dbCallback(unsigned gameNum, float percentComplete, void *contextInfo);

bool playGames(const string &engineId1, const string &engineId2) {
    Engine engines[2];
    shared_ptr<Database> outdb;
    shared_ptr<OpeningTree> openingTree;
    shared_ptr<EngineMessage> message;

    if (!g_optOutputDb.empty()) {
        shared_ptr<Database> outdb = Database::openDatabase(g_optOutputDb, false);
        if (outdb.get() == 0) {
            cerr << "Don't know how to create database '" << g_optOutputDb << "'" << endl;
            return false;
        } else if (!outdb->isOpen()) {
            cerr << outdb->errorMsg() << endl;
            return false;
        }

        cout << "Indexing database '" << g_optOutputDb << "'" << endl;

        if (outdb->needsIndexing() &&
            !outdb->index(dbCallback, NULL)) {
            cerr << outdb->errorMsg() << endl;
            return false;
        }
    }

    if (!g_optEcoFile.empty()) {
        openingTree.reset(new OpeningTree(g_optEcoFile));

        if (openingTree->isOpen())
            cout << "Opened ECO classification file '" << g_optEcoFile << "'" << endl;
        else {
            cerr << "Failed to open ECO classification file '" << g_optEcoFile << "'" << endl;
            return false;
        }
    }

    const shared_ptr<Config> config1 = Config::config(engineId1);
    if (config1.get() == 0) {
        cerr << "Engine '" << engineId1 << "' is not configured" << endl;
        return false;
    }

    const shared_ptr<Config> config2 = Config::config(engineId2);
    if (config2.get() == 0) {
        cerr << "Engine '" << engineId2 << "' is not configured" << endl;
        return false;
        
    }

    engines[0].setId(engineId1);
    engines[1].setId(engineId2);

    if (g_optLogComms) {
        engines[0].setUciDebug(engineUciDebug, 0);
        engines[1].setUciDebug(engineUciDebug, 0);
    }

    if (!engines[0].load(Util::expandEnv(config1->cmdLine()), Util::expandEnv(config1->workDir()),
                         config1->startupTimeout(), config1->timeout())) {
        cerr << "Failed to load engine " << engineId1 << endl;
        return false;
    }

    for (auto it = config1->options().begin(); it != config1->options().end(); ++it)
        engines[0].enqueueMessage(NEW_ENGINE_MESSAGE_SET_OPTION(it->first, it->second));

    if (!engines[1].load(Util::expandEnv(config2->cmdLine()), Util::expandEnv(config2->workDir()),
                         config2->startupTimeout(), config2->timeout())) {
        cerr << "Failed to load engine " << engineId2 << endl;
        return false;
    }

    for (auto it = config2->options().begin(); it != config2->options().end(); ++it)
        engines[1].enqueueMessage(NEW_ENGINE_MESSAGE_SET_OPTION(it->first, it->second));

    Tournament tourney;
    EngineScore engine1Score, engine2Score;
    string gameOverReason;
    unsigned dotFileIndex = 1;

    tourney.numRounds = g_optNumber1;

    for (tourney.gameNum = 1; tourney.gameNum <= tourney.numRounds && !g_quitFlag; tourney.gameNum++) {
        EngineScore *whiteScore, *blackScore;

        if ((tourney.gameNum & 1) == 1) {
            tourney.white = &engines[0];
            tourney.black = &engines[1];
            whiteScore = &engine1Score;
            blackScore = &engine2Score;
        } else {
            tourney.white = &engines[1];
            tourney.black = &engines[0];
            whiteScore = &engine2Score;
            blackScore = &engine1Score;
        }

        tourney.game.init();
        tourney.game.setEvent("Engine Tournament");
        tourney.game.setSiteComputer();
        tourney.game.white().setLastName(tourney.white->name());
        tourney.game.black().setLastName(tourney.black->name());
        tourney.game.setDateNow();
        tourney.game.setRoundMajor(tourney.gameNum);
        tourney.game.setRoundMinor(0);
        tourney.game.setTimeControl(g_optTimeControl);

        cout << "**********************************************************************" << endl;
        cout << "game " << dec << tourney.gameNum << ": " << tourney.white->id() << " vs. "
             << tourney.black->id() << endl;
        cout << "**********************************************************************" << endl;

        bool gameOK = playGame(tourney, gameOverReason);

        if (gameOK)
            cout << "Game Over: " << gameOverReason << endl;
        else {
            if (g_quitFlag)
                cout << "Tournament abandoned by user" << endl;
            else
                cout << "Tournament abandoned due to error: " << gameOverReason << endl;

            tourney.game.setResult(Game::UNFINISHED);
        }

        switch (tourney.game.result()) {
        case Game::WHITE_WIN:
            whiteScore->wins++;
            blackScore->loses++;
            break;
        case Game::BLACK_WIN:
            whiteScore->loses++;
            blackScore->wins++;
            break;
        case Game::DRAW:
            whiteScore->draws++;
            blackScore->draws++;
            break;
        default:
            break;
        }

        if (outdb && outdb->isOpen()) {
            if (openingTree && openingTree->isOpen())
                openingTree->classify(tourney.game);

            if (!outdb->write(outdb->numGames() + 1, tourney.game))
                cerr << "Failed to write game to database: " << outdb->errorMsg() << endl;
        }

        // Print out the current tournament result even if we are
        // exiting
        cout << engines[0].id() << " score: +" << dec << engine1Score.wins << "/-" << dec
             << engine1Score.loses << "/=" << dec << engine1Score.draws << " (" << fixed
             << setprecision(1) << (float(engine1Score.wins) + float(engine1Score.draws) * 0.5f)
             << "), " << engines[1].id() << " score: +" << dec << engine2Score.wins << "/-" << dec
             << engine2Score.loses << "/=" << dec << engine2Score.draws << " (" << fixed
             << setprecision(1) << (float(engine2Score.wins) + float(engine2Score.draws) * 0.5f)
             << ")" << endl;

        // Dump the final game tree to .dot file
        if (!g_optDotDir.empty()) {
            string dotFileName = Util::format("%s/game_%08u.dot", g_optDotDir.c_str(), dotFileIndex++);

            if (!AnnotMove::writeToDotFile(tourney.game.mainline(), dotFileName))
                cerr << "Failed to write game tree to file '" << dotFileName << "'" << endl;
        }

        if (!gameOK)
            break;
    }

    cout << "Unloading engines" << endl;
    engines[0].unload();
    engines[1].unload();

    return true;
}

static bool playGame(Tournament &tourney, string &gameOverReason) {
    enum EventIndex {
        EventIndexWhiteEngine,
        EventIndexBlackEngine,
        EventIndexQuit,
        //
        EventSize
    };

    IoEventList e(EventSize);
    IoEventWaiter waiter;
    shared_ptr<EngineMessage> message;
    TimeTracker whiteTimeTracker(g_optTimeControl), blackTimeTracker(g_optTimeControl);
    bool whiteToPlay, haveMove, whiteTimedOut = false, blackTimedOut = false;
    int waitResult, lastScore, lastMateScore;
    unsigned thinkingTime;
    ostringstream ss;
    string score, formattedMove;
    Move move;
    Game::GameOver gameover = Game::GAMEOVER_NOT;

    gameOverReason.clear();

    loginf("Starting game %s vs. %s (%u of %u)",
           tourney.white->id().c_str(), tourney.black->id().c_str(),
           tourney.gameNum, tourney.numRounds);

    ASSERT(tourney.white->isLoaded());
    ASSERT(tourney.black->isLoaded());
    ASSERT(tourney.white->isThreadRunning());
    ASSERT(tourney.black->isThreadRunning());

    tourney.white->resetQueues();
    tourney.black->resetQueues();

    tourney.white->enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_NEW_GAME));
    tourney.black->enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_NEW_GAME));

    e[EventIndexWhiteEngine] = &(tourney.white->fromQueue().event());
    e[EventIndexBlackEngine] = &(tourney.black->fromQueue().event());
    e[EventIndexQuit] = &g_quitEvent;

    if (!waiter.setEvents(e)) {
        cerr << "Failed to set waiter events" << endl;
        return false;
    }

    if (g_optTimeControl.isValid()) {
        cout << "Engines using time control '" << g_optTimeControl.notation() << "'" << endl;
        tourney.white->setWhiteTimeTracker(&whiteTimeTracker);
        tourney.white->setBlackTimeTracker(&blackTimeTracker);
        tourney.white->resetTimeTrackers();
        tourney.black->setWhiteTimeTracker(&whiteTimeTracker);
        tourney.black->setBlackTimeTracker(&blackTimeTracker);
        tourney.black->resetTimeTrackers();
    } else if (g_optDepth > 0) {
        cout << "Engines using think depth " << g_optDepth << endl;
        tourney.white->setThinkDepth(g_optDepth);
        tourney.black->setThinkDepth(g_optDepth);
    } else {
        cerr << "Neither time control nor depth specified; the engines will think for 1 second per move" << endl;
    }

    whiteToPlay = true;

    cout << "Starting position:\n" << tourney.game.position() << endl;

    while (gameover == Game::GAMEOVER_NOT && !g_quitFlag) {
        Engine *toMove = whiteToPlay ? tourney.white : tourney.black;

        // Check the health of the engine
        if (!toMove->isLoaded()) {
            cerr << "Engine " << toMove->id() << ": Engine process not loaded!" << endl;
            return false;
        } else if (!toMove->isThreadRunning()) {
            cerr << "Engine " << toMove->id() << ": I/O thread has stopped running!" << endl;
            return false;
        }

        // Set the position
        vector<Move> moves;
        tourney.game.moveList(tourney.game.currentMove(), moves);
        toMove->enqueueMessage(NEW_ENGINE_MESSAGE_POSITION(tourney.game.position(),
                                                       tourney.game.startPosition(), moves));

        lastScore = 0;
        lastMateScore = 0;

        // Start the engine thinking
        toMove->enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_GO));

        if (toMove->validTimeTrackers()) {
            if (whiteToPlay) {
                cout << "White (" << toMove->id() << ") to play [" <<
                    Util::formatElapsed(toMove->whiteTimeTracker()->timeLeft()) << "] " <<
                    Util::formatElapsed(toMove->blackTimeTracker()->timeLeft()) << endl;
            } else {
                cout << "Black (" << toMove->id() << ") to play " <<
                    Util::formatElapsed(toMove->whiteTimeTracker()->timeLeft()) << " [" <<
                    Util::formatElapsed(toMove->blackTimeTracker()->timeLeft()) << "]" << endl;
            }
        } else {
            if (whiteToPlay)
                cout << "White (" << toMove->id() << ") to play" << endl;
            else
                cout << "Black (" << toMove->id() << ") to play" << endl;
        }

        haveMove = false;

        while (!haveMove && !g_quitFlag) {
            waitResult = waiter.wait(ENGINE_WAIT_TIMEOUT);

            if (waitResult < 0) {
                // IoEvent::wait() error or timeout
                gameOverReason = Util::format("%s while waiting for an engine to respond",
                                              (waitResult == -1 ? "Error" : "Timeout"));
                return false;
            }

            while (waitResult >= 0) {
                // Something to read
                if (waitResult == EventIndexWhiteEngine) {
                    message = tourney.white->dequeueMessage();

                    if (message.get() == 0) {
                        waitResult = IO_EVENT_WAIT_ERROR;
                        continue;
                    }

                    if (!whiteToPlay)
                        continue; // Not interested

                } else if (waitResult == EventIndexBlackEngine) {
                    message = tourney.black->dequeueMessage();

                    if (message.get() == 0) {
                        waitResult = IO_EVENT_WAIT_ERROR;
                        continue;
                    } else if (whiteToPlay)
                        continue; // Not interested

                } else if (waitResult == EventIndexQuit) {
                    // Quit event
                    logdbg("Quit event signalled");
                    g_quitFlag = true;
                    return false;
                } else { // Shouldn't happen
                    ASSERT(false);
                    return false;
                }

                switch (message->type) {
                case EngineMessage::TYPE_BEST_MOVE: {
                    EngineMessageBestMove *engineMessageBestMove =
                        dynamic_cast<EngineMessageBestMove *> (message.get());
                    move = engineMessageBestMove->bestMove;
                    thinkingTime = engineMessageBestMove->thinkingTime;

                    // Check we got the best move from the correct engine
                    if ((whiteToPlay && waitResult != EventIndexWhiteEngine) ||
                        (!whiteToPlay && waitResult != EventIndexBlackEngine)) {
                        ss.str("");
                        ss << "Got a 'bestmove' reply from the wrong engine!";

                        if (g_optRelaxed)
                            cerr << ss.str() << endl;
                        else {
                            gameOverReason = ss.str();
                            return false;
                        }
                    }

                    haveMove = true;
                    break;
                }
                case EngineMessage::TYPE_INFO_SEARCH: {
                    displayInfo(message, lastScore, lastMateScore);
                    break;
                }
                case EngineMessage::TYPE_INFO_STRING: {
                    EngineMessageInfoString *engineMessageInfoString = dynamic_cast<EngineMessageInfoString *>
                                                                       (message.get());
                    cout << toMove->id() << ": " << engineMessageInfoString->info << endl;
                    break;
                }
                case EngineMessage::TYPE_ERROR: {
                    EngineMessageError *engineMessageError = dynamic_cast<EngineMessageError *> (message.get());
                    ss.str("");
                    ss << engineMessageError->error;

                    if (g_optRelaxed)
                        cerr << ss.str() << endl;
                    else {
                        gameOverReason = ss.str();
                        return false;
                    }

                    break;
                }
                default: {
                    LOGDBG << "Ignoring message " << EngineMessage::typeDesc(message->type) <<
                        " from engine " << toMove->id();
                    break;
                }

                }
            }
        }

        if (g_quitFlag)
            return false;

        // Manage time
        // (use the score from engine as the move annotation, by default)
        if (toMove->validTimeTrackers()) {
            if (toMove->thinkingAsWhite() && toMove->whiteTimeTracker()->isOutOfTime()) {
                whiteTimedOut = true;
                tourney.game.setResult(Game::BLACK_WIN);
                score = "Lost on time";
                gameOverReason = Util::format("White (%s) lost on time", tourney.white->id().c_str());
            } else if (!toMove->thinkingAsWhite() && toMove->blackTimeTracker()->isOutOfTime()) {
                blackTimedOut = true;
                tourney.game.setResult(Game::WHITE_WIN);
                score = "Lost on time";
                gameOverReason = Util::format("Black (%s) lost on time", tourney.black->id().c_str());
            }
        } else {
            if (lastMateScore)
                score = Util::format("#%d", lastMateScore);
            else
                score = Util::formatCenti(lastScore);
        }

        if (!tourney.game.makeMove(move, &score, &formattedMove, true, &gameover)) {
            gameOverReason = Util::format("Invalid move '%s' from %s",
                                          move.dump().c_str(), toMove->id().c_str());
            return false;
        }

        if (gameover != Game::GAMEOVER_NOT) {
            switch (gameover) {
            case Game::GAMEOVER_MATE:

                if (whiteToPlay) {
                    tourney.game.setResult(Game::WHITE_WIN);
                    gameOverReason = Util::format("White (%s) gave mate",
                                                  tourney.white->id().c_str());
                } else {
                    tourney.game.setResult(Game::BLACK_WIN);
                    gameOverReason = Util::format("Black (%s) gave mate",
                                                  tourney.black->id().c_str());
                }

                break;
            case Game::GAMEOVER_STALEMATE:
                tourney.game.setResult(Game::DRAW);
                gameOverReason = "Stalemate";
                break;
            case Game::GAMEOVER_50MOVERULE:
                tourney.game.setResult(Game::DRAW);
                gameOverReason = "Draw by 50-move rule";
                break;
            case Game::GAMEOVER_3FOLDREP:
                tourney.game.setResult(Game::DRAW);
                gameOverReason = "Draw by 3-fold repetition";
                break;
            case Game::GAMEOVER_NOMATERIAL:
                tourney.game.setResult(Game::DRAW);
                gameOverReason = "Draw by insufficient material";
                break;
            default:
                ASSERT(false);
                break;
            }
        } else if (whiteTimedOut) {
            gameover = Game::GAMEOVER_TIME;
            tourney.game.setResult(Game::BLACK_WIN);
        } else if (blackTimedOut) {
            gameover = Game::GAMEOVER_TIME;
            tourney.game.setResult(Game::WHITE_WIN);
        }

        // Print out move
        string formattedTime = Util::formatElapsed(thinkingTime);
        cout << (whiteToPlay ? "White" : "Black") << " (" << toMove->id() << ") moved "
             << formattedMove << " time: " << formattedTime << " score: "
             << score << endl << tourney.game.position() << endl;
        logdbg("%s (%s) moved %s time: %s score: %s", (whiteToPlay ? "White" : "Black"),
               toMove->id().c_str(), formattedMove.c_str(), formattedTime.c_str(), score.c_str());

        // Log the new position
        // cout << "Position after " << fmtd_move << ":\n" << tourney.game.get_position() << endl;

        whiteToPlay = !whiteToPlay;
    }

    if (!g_quitFlag)
        loginf("Game finished: %s", gameOverReason.c_str());

    return !g_quitFlag;
}

static bool displayInfo(const shared_ptr<EngineMessage> &message, int &score, int &mateScore) {
    EngineMessageInfoSearch *info = dynamic_cast<EngineMessageInfoSearch *> (message.get());

    if (info->have & EngineMessageInfoSearch::HAVE_SCORE)
        score = info->score;

    if (info->have & EngineMessageInfoSearch::HAVE_MATESCORE)
        mateScore = info->mateScore;

    if ((info->have & EngineMessageInfoSearch::HAVE_PV) == 0)
        return true;    // Only interested in info containing pv

    cout << info->format() << endl;
    return true;
}

static void engineUciDebug(void *userp, const Engine *engine, bool fromEngine, const std::string &message) {
    if (fromEngine)
        LOGDBG << "<" << engine->id() << " " << message;
    else
        LOGDBG << engine->id() << "> " << message;
}

static bool dbCallback(unsigned gameNum, float percentComplete, void *contextInfo) {
    if ((gameNum % 1000) == 0)
        cout << gameNum << " (" << percentComplete << "%)" << endl;

    return !g_quitFlag;
}
