//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// AnalyzeGames.cpp: Analyze game(s) using an engine.
//

#include "ccore.h"
#include <ChessCore/Engine.h>
#include <ChessCore/Database.h>
#include <ChessCore/PgnDatabase.h>    // In order to call index()
#include <ChessCore/Log.h>
#include <ChessCore/IoEventWaiter.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <errno.h>

using namespace std;
using namespace ChessCore;

static const char *m_classname = "AnalyzeGames";

// Maximum we will wait before getting bored
const int ENGINE_WAIT_TIMEOUT = 60 * 1000;

static bool analyzeGame(int gameNum, Game &game, Engine &engine);
static bool displayInfo(const shared_ptr<EngineMessage> &message, int &score, int &mateScore, vector<Move> &pv);
static void engineUciDebug(void *userp, const Engine *engine, bool fromEngine, const std::string &message);
static bool dbCallback(unsigned gameNum, float percentComplete, void *contextInfo);

bool analyzeGames(const string &engineId) {
    Engine engine;
    shared_ptr<Database> indb, outdb;
    Game game;
    unsigned inGameNum, outGameNum, firstGame = 0, lastGame = 0, dotFileIndex = 1;
    bool retval = true;

    if (!g_optInputDb.empty()) {
        indb = Database::openDatabase(g_optInputDb, false);
        if (indb.get() == 0) {
            cerr << "Don't know how to read database '" << g_optInputDb << "'" << endl;
            return false;
        } else if (!indb->isOpen()) {
            cerr << indb->errorMsg() << endl;
            return false;
        }

        if (indb->needsIndexing() &&
            !indb->index(dbCallback, NULL)) {
            cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
            return false;
        }

        if (indb->numGames() == 0) {
            cerr << "Database '" << g_optInputDb << "' is empty" << endl;
            return false;
        }
    } else {
        cerr << "No input database specified" << endl;
        return false;
    }

    if (!g_optOutputDb.empty()) {
        outdb = Database::openDatabase(g_optOutputDb, false);
        if (outdb.get() == 0) {
            cerr << "Don't know how to create database '" << g_optOutputDb << "'" << endl;
            return false;
        } else if (!outdb->isOpen()) {
            cerr << outdb->errorMsg() << endl;
            return false;
        }

        if (outdb->needsIndexing() &&
            !outdb->index(dbCallback, NULL)) {
            cerr << "Failed to index database '" << g_optOutputDb << "': " << outdb->errorMsg() << endl;
            return false;
        }
    } else {
        cerr << "No output database specified" << endl;
        return false;
    }

    if (g_optTime == 0 && g_optDepth == 0) {
        cerr << "No time or depth control specified" << endl;
        return false;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    const shared_ptr<Config> config = Config::config(engineId);
    if (config.get() == 0) {
        cerr << "Engine '" << engineId << "' is not configured" << endl;
        return false;
    }

    engine.setId(engineId);

    if (g_optLogComms)
        engine.setUciDebug(engineUciDebug, 0);

    if (!engine.load(Util::expandEnv(config->cmdLine()), Util::expandEnv(config->workDir()),
                     config->startupTimeout(), config->timeout())) {
        cerr << "Failed to load engine " << engineId << endl;
        return false;
    }

    const ChessCore::StringStringMap options = config->options();

    for (auto it = options.begin(); it != options.end(); ++it)
        engine.enqueueMessage(NEW_ENGINE_MESSAGE_SET_OPTION(it->first, it->second));

    // Use UCI_AnalyseMode, if the engine supports it
    const StringOptionMap &optionMap = engine.engineOptions();

    if (optionMap.find("UCI_AnalyseMode") != optionMap.end())
        engine.enqueueMessage(NEW_ENGINE_MESSAGE_SET_OPTION("UCI_AnalyseMode", "true"));

    cout << "Analyzing games " << firstGame << "-" << lastGame << " in database '" <<
        g_optInputDb << "'. Writing analysis to database '" << g_optOutputDb << "'" << endl;

    for (inGameNum = firstGame, outGameNum = outdb->numGames() + 1;
         inGameNum <= lastGame && retval;
         inGameNum++, outGameNum++) {
        if (!indb->gameExists(inGameNum)) {
            cout << "Game " << inGameNum << " does not exist" << endl;
            continue;
        }

        retval = indb->read(inGameNum, game);

        if (retval) {
            retval = analyzeGame(inGameNum, game, engine);

            if (retval) {
                retval = outdb->write(outGameNum, game);

                if (!retval)
                    cerr << "Failed to write game " << outGameNum << ": " << outdb->errorMsg() << endl;

                // Dump the final game tree to .dot file
                if (!g_optDotDir.empty()) {
                    string dotFileName = Util::format("%s/game_%08u.dot", g_optDotDir.c_str(), dotFileIndex++);

                    if (!AnnotMove::writeToDotFile(game.mainline(), dotFileName))
                        cerr << "Failed to write game tree to file '" << dotFileName << "'" << endl;
                }
            } else {
                cerr << "Failed to analyze game " << inGameNum << endl;
            }
        } else {
            cerr << "Failed to read game " << inGameNum << ": " << indb->errorMsg() << endl;
        }
    }

    if (retval)
        cout << "Successfully analyzed database games" << endl;
    else
        cout << "Failed to analyze database games" << endl;

    engine.unload();

    return true;
}

static bool analyzeGame(int gameNum, Game &game, Engine &engine) {
    IoEventWaiter waiter;
    IoEventList e(2);
    shared_ptr<EngineMessage> message;
    int lastScore, lastMateScore, waitResult;
    string str, str1, str2, score, formattedMove;
    ostringstream ss;
    Move bestMove;
    AnnotMove *amove;
    vector<string> parts;
    vector<Move> pv;
    Game::GameOver gameover = Game::GAMEOVER_NOT;

    cout << "Analyzing game " << gameNum <<
        " '" << game.white().formattedName() <<
        "' vs. '" << game.black().formattedName() << "'" << endl;

    AnnotMove::removeVariations(game.mainline());

    ASSERT(engine.isLoaded());

    engine.enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_NEW_GAME));

    e[0] = &(engine.fromQueue().event());
    e[1] = &g_quitEvent;

    if (!waiter.setEvents(e)) {
        cerr << "Failed to set waiter events";
        return false;
    }

    game.setCurrentMove(0);
    for (amove = game.mainline();
         amove != 0 && gameover == Game::GAMEOVER_NOT && !g_quitFlag;
         amove = amove->next()) {
        pv.clear();
        lastScore = 0;
        lastMateScore = 0;

        cout << endl << game.position() << endl;
        cout << "Analyzing " << game.position().moveNumber() << amove->san(game.position()) << endl;

        // Generate the "position ..." string (we use m_prev as we want the engine
        // to suggest a possible alternative move to the one that was actually made)
        Position startPosition;
        vector<Move> moves;
        game.moveList(amove->prev(), moves);
        engine.enqueueMessage(NEW_ENGINE_MESSAGE_POSITION(game.position(), game.startPosition(), moves));

        engine.timeControl().clear();

        if (g_optTime > 0)
            engine.timeControl().moveTime = g_optTime * 1000;

        if (g_optDepth > 0)
            engine.timeControl().depth = g_optDepth;

        engine.enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_GO));

        bestMove.init();

        while (bestMove.isNull() && !g_quitFlag) {
            waitResult = waiter.wait(ENGINE_WAIT_TIMEOUT);
            if (waitResult >= 0) {
                // Something to read
                if (waitResult == 0) {
                    message = engine.dequeueMessage();
                    if (message.get() == 0) {
                        cerr << "Failed to read an expected message from engine %s" << engine.id().c_str() << endl;
                        return false;
                    }
                } else if (waitResult == 1) {
                    // Quit event
                    logdbg("Quit event signalled");
                    return false;
                } else { // Shouldn't happen
                    ASSERT(false);
                    return false;
                }

                switch (message->type) {
                case EngineMessage::TYPE_BEST_MOVE: {
                    EngineMessageBestMove *engineMessageBestMove = dynamic_cast<EngineMessageBestMove *>(message.get());
                    bestMove = engineMessageBestMove->bestMove;
                    break;
                }
                case EngineMessage::TYPE_INFO_SEARCH: {
                    displayInfo(message, lastScore, lastMateScore, pv);
                    break;
                }
                case EngineMessage::TYPE_INFO_STRING: {
                    EngineMessageInfoString *engineMessageInfoString = dynamic_cast<EngineMessageInfoString *>(message.get());
                    LOGINF << engine.id() << ": " << engineMessageInfoString->info;
                    break;
                }
                case EngineMessage::TYPE_ERROR: {
                    EngineMessageError *engineMessageError = dynamic_cast<EngineMessageError *> (message.get());
                    LOGERR << engine.id() << ": " << engineMessageError->error;
                    break;
                }
                default: {
                    LOGDBG << "Ignoring message " << EngineMessage::typeDesc(message->type) <<
                        " from engine " << engine.id();
                    break;
                }
                }
            } else {
                // Error or timeout
                cerr << (waitResult == -1 ? "Error" : "Timeout") << " while waiting for an engine to respond" << endl;
                return false;
            }
        }

        if (g_quitFlag)
            return false;

        ASSERT(!bestMove.isNull());

        if (lastMateScore != 0)
            score = Util::format("#%d", lastMateScore);
        else
            score = Util::formatCenti(lastScore);

        if (bestMove.equals(*amove)) {
            // Score the move that was actually made
            cout << "Engine " << engine.id() << " agrees with move " <<
                game.position().moveNumber() << amove->san(game.position()) << " with score " << score << endl;
            amove->setPostAnnot(score);
            game.setCurrentMove(amove);
        } else {
            // Make the engines move as a variation
            if (pv.size() > 0 && bestMove.equals(pv[0])) {
                // Last PV from the engine matches the bestmove it returned
                cout << "Engine " << engine.id() << " prefers move " <<
                    game.position().moveNumber() << pv[0].san(game.position()) << " with score " << score << endl;

                game.setCurrentMove(amove);
                AnnotMove *variation = game.addVariation(pv);
                if (variation == 0) {
                    LOGERR << "Failed to add variation " << Move::dump(pv) << " in position:\n" << game.position();
                    return false;
                }
                variation->lastMove()->setPostAnnot(score);
            } else { // Best is not the last PV we received
                // Add this move as a variation then
                cout << "Engine " << engine.id() << " prefers move " <<
                    game.position().moveNumber() << bestMove.san(game.position()) << " with score " << score << endl;

                vector<Move> moveList;
                moveList.push_back(bestMove);
                game.setCurrentMove(amove);
                AnnotMove *variation = game.addVariation(moveList);
                if (variation == 0) {
                    LOGERR << "Failed to add variation " << bestMove.dump() << " in position:\n" << game.position();
                    return false;
                }
                variation->lastMove()->setPostAnnot(score);
                cout << "Engine " << engine.id() << " prefers move " <<
                    game.position().moveNumber() << bestMove.san(game.position()) << " with score " << score << endl;
            }
        }

        gameover = game.isGameOver();
    }

    return !g_quitFlag;
}

static bool displayInfo(const shared_ptr<EngineMessage> &message, int &score, int &mateScore, vector<Move> &pv) {
    EngineMessageInfoSearch *info = dynamic_cast<EngineMessageInfoSearch *> (message.get());

    if ((info->have & EngineMessageInfoSearch::HAVE_SCORE) != 0)
        score = info->score;

    if ((info->have & EngineMessageInfoSearch::HAVE_MATESCORE) != 0)
        mateScore = info->mateScore;

    if ((info->have & EngineMessageInfoSearch::HAVE_PV) == 0)
        return true;    // Only interested in info containing pv

    pv.clear();
    pv.assign(info->pv.begin(), info->pv.end());

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
