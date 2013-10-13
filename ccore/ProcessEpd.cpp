//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ProcessEpd.cpp: Process EPD file.
//

#include "ccore.h"
#include <ChessCore/Log.h>
#include <ChessCore/Engine.h>
#include <ChessCore/Epd.h>
#include <ChessCore/Database.h>
#include <iostream>
#include <sstream>
#include <memory>

using namespace std;
using namespace ChessCore;

static const char *m_classname = "ProcessEpd";

static bool setPosition(Engine &engine, const Position &pos);
static bool getPerft(Engine &engine, const Position &pos, unsigned depth, int64_t &nodes, int64_t &nps);
static bool getEval(Engine &engine, const Position &pos, int32_t &score);
static bool getBestMove(Engine &engine, const Position &pos, Move &move);
static bool displayInfo(const shared_ptr<EngineMessage> &message);
static void engineUciDebug(void *userp, const Engine *engine, bool fromEngine, const std::string &message);

bool processEpd(const string &engineId) {
    unsigned failCount = 0, successCount = 0, depth;
    string message, str, str2, opname;
    unsigned j, start, end, timeTaken;
    unsigned first, last;
    int64_t nodes, nps;
    int32_t score, evalSlightAdv = 0, evalClearAdv = 0;
    Engine engine;
    Move move;
    bool pass;
    EpdOp::Eval eval;

    if (g_optEpdFile.empty()) {
        cerr << "No EPD file specified" << endl;
        return false;
    }

    if (g_optTimeControl.isValid()) {
        // Time control must has a single "moves in" period only
        if (g_optTimeControl.periods().size() != 1 ||
            g_optTimeControl.periods()[0].type() != TimeControlPeriod::TYPE_MOVES_IN) {
            cerr << "Time controld must contain a single 'Moves In' period" << endl;
            return false;
        }
    }

    EpdFile epdFile;

    if (!epdFile.readFromFile(g_optEpdFile)) {
        cerr << "Failed to open file EPD file '" << g_optEpdFile << "'" << endl;
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

    if (g_optNumber1 > 0)
        first = g_optNumber1 - 1;
    else
        first = 0;

    if (g_optNumber2 > 0 && g_optNumber2 >= g_optNumber1 && g_optNumber2 < (int)epdFile.numEpds())
        last = g_optNumber2 - 1;
    else
        last = (int)epdFile.numEpds() - 1;

    unsigned startTime = Util::getTickCount();

    for (j = first; j <= last && !g_quitFlag; j++) {
        Epd *epd = epdFile.epd(j);
        ASSERT(epd);
        const EpdOp *op = epd->findFirstOp("id");

        if (op) {
            cout << "Processing " << g_optEpdFile << ":" << dec << epd->lineNum() << " '"
                 << op->operandString() << "'\n";
        } else {
            cout << "Processing " << g_optEpdFile << ":" << dec << epd->lineNum() << "\n";
        }

        cout << *epd << endl;

        //
        // 'Perft' processing - only chimp supports this
        //
        op = epd->findFirstOp("perft1");

        if (op) {
            if (engine.name().find("Chimp") != string::npos) {
                depth = 1;

                do {
                    if (!setPosition(engine, epd->pos()))
                        return false;

                    start = Util::getTickCount();

                    if (!getPerft(engine, epd->pos(), depth, nodes, nps))
                        return false;

                    end = Util::getTickCount();
                    timeTaken = end - start;

                    pass = nodes == op->operandInteger();

                    if (pass)
                        successCount++;
                    else
                        failCount++;

                    cout << engine.id() << " gives " << dec << nodes << " nodes in "
                         << Util::formatElapsed(timeTaken) << " ("
                         << Util::formatNPS(nodes, timeTaken) << "): " << (pass ? "PASS" : "FAIL")
                         << " (" << dec << successCount << "/" << dec << (successCount + failCount)
                         << ")" << endl;

                    if (!pass)
                        break;    // If this failed then the next will too

                    opname = Util::format("perft%d", ++depth);
                } while (!g_quitFlag && (op = epd->findNextOp(opname)));
            } else {
                cerr << "Skipping perft tests as Chimp engine is not loaded" << endl;
            }
        }

        //
        // 'Eval' processing
        //
        op = epd->findFirstOp("eval");

        if (op) {
            if (evalSlightAdv == 0)
                evalSlightAdv = 75;

            if (evalClearAdv == 0)
                evalClearAdv = 150;

            if (!setPosition(engine, epd->pos()))
                return false;

            start = Util::getTickCount();

            if (!getEval(engine, epd->pos(), score))
                return false;

            end = Util::getTickCount();
            timeTaken = end - start;

            // Normalise the score from the 'engines point of view' to absolute white/black
            if (toColour(epd->pos().ply()) == WHITE)
                score = -score;

            // Check where the score lies compared to the evaluation in the EPD operand
            eval = EpdOp::EVAL_NONE;

            if (score == 0)
                eval = EpdOp::EVAL_EQUAL;
            else if (score > 0) {
                if (score < evalSlightAdv)
                    eval = EpdOp::EVAL_W_SLIGHT_ADV;
                else if (score < evalClearAdv)
                    eval = EpdOp::EVAL_W_CLEAR_ADV;
                else
                    eval = EpdOp::EVAL_W_DECISIVE_ADV;
            } else { // score < 0
                if (score > -evalSlightAdv)
                    eval = EpdOp::EVAL_B_SLIGHT_ADV;
                else if (score > -evalClearAdv)
                    eval = EpdOp::EVAL_B_CLEAR_ADV;
                else
                    eval = EpdOp::EVAL_B_DECISIVE_ADV;
            }

            pass = eval == op->operandEval();

            if (pass)
                successCount++;
            else
                failCount++;

            cout << engine.id() << " gives score=" << ((int)score < 0 ? '-' : '+') << (int)score
                 << " (" << EpdOp::formatEval(eval) << ") in " << Util::formatElapsed(timeTaken)
                 << ": " << (pass ? "PASS" : "FAIL") << " (" << dec << successCount << "/" << dec
                 << (successCount + failCount) << ")" << endl;
        }

        //
        // Move processing
        //
        if (epd->hasMoveOps()) {
            if (!setPosition(engine, epd->pos()))
                return false;

            start = Util::getTickCount();

            if (!getBestMove(engine, epd->pos(), move))
                return false;

            end = Util::getTickCount();
            timeTaken = end - start;

            pass = epd->checkMoveOps(move);

            if (pass)
                successCount++;
            else
                failCount++;

            cout << engine.id() << " gives " << move.san(epd->pos()) << " in "
                 << Util::formatElapsed(timeTaken) << ": " << (pass ? "PASS" : "FAIL") << " (" << dec
                 << successCount << "/" << dec << (successCount + failCount) << ")" << endl;
        }

        cout << "-----------------------------------" << endl;
    }

    unsigned endTime = Util::getTickCount();

    cout << dec << successCount << " succeeded, " << dec << failCount << " failed in "
         << Util::formatElapsed(endTime - startTime) << "s" << endl;

    engine.unload();

    return failCount == 0;
}

//
// Reset the engine and set-up the position from the EPD file
//
static bool setPosition(Engine &engine, const Position &pos) {
    ostringstream oss;
    vector<Move> moves;

    engine.enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_NEW_GAME));
    engine.enqueueMessage(NEW_ENGINE_MESSAGE_POSITION(pos, pos, moves));
    return true;
}

//
// Start the engine calculating the perft of the position and wait
// for answer
//
static bool getPerft(Engine &engine, const Position &pos, unsigned depth, int64_t &nodes, int64_t &nps) {
    int waitResult;
    bool haveNodes, haveNps;
    ostringstream oss;

    IoEventList e(2);
    IoEventWaiter waiter;
    shared_ptr<EngineMessage> message;

    e[0] = &(engine.fromQueue().event());
    e[1] = &g_quitEvent;
    waiter.setEvents(e);
    {
        cerr << "Failed to set waiter events";
        return false;
    }

    engine.enqueueMessage(NEW_ENGINE_MESSAGE_CUSTOM(Util::format("test perft %u", depth)));

    haveNodes = haveNps = false;

    while (!haveNodes && !haveNps && !g_quitFlag) {
        waitResult = waiter.wait(-1);

        if (waitResult == 0) {
            message = engine.dequeueMessage();

            if (message.get() == 0) {
                cerr << "Failed to read an expected message from engine " << engine.id() << endl;
                return false;
            }

            switch (message->type) {
            case EngineMessage::TYPE_INFO_SEARCH: {
                EngineMessageInfoSearch *engineMessageInfoSearch = dynamic_cast<EngineMessageInfoSearch *>
                                                                   (message.get());

                if (engineMessageInfoSearch->have & EngineMessageInfoSearch::HAVE_NODES) {
                    nodes = engineMessageInfoSearch->nodes;
                    haveNodes = true;
                }

                if (engineMessageInfoSearch->have & EngineMessageInfoSearch::HAVE_NPS) {
                    nps = engineMessageInfoSearch->nps;
                    haveNps = true;
                }

                break;
            }
            case EngineMessage::TYPE_INFO_STRING: {
                EngineMessageInfoString *engineMessageInfoString = dynamic_cast<EngineMessageInfoString *>
                                                                   (message.get());
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

            if (!haveNodes || !haveNps) {
                cerr << "No nodes or nps entry in engine output" << endl;
                return false;
            }
        } else {
            // Quit or error
            return false;
        }
    }

    return g_quitFlag ? false : true;
}

//
// Start the engine calculating the eval of the position and wait
// for answer
//
static bool getEval(Engine &engine, const Position &pos, int32_t &score) {
    string str;
    int waitResult;
    bool haveScore, haveMove;
    shared_ptr<EngineMessage> message;
    TimeTracker whiteTimeTracker(g_optTimeControl), blackTimeTracker(g_optTimeControl);
    Move move;

    IoEventList e(2);
    IoEventWaiter waiter;
    e[0] = &(engine.fromQueue().event());
    e[1] = &g_quitEvent;

    if (!waiter.setEvents(e)) {
        cerr << "Failed to set waiter events";
        return false;
    }

    if (g_optTimeControl.isValid()) {
        cout << "Engine using time control '" << g_optTimeControl.notation(TimeControlPeriod::FORMAT_PGN) << "'" << endl;
        engine.setTimeTrackers(&whiteTimeTracker, &blackTimeTracker);
        engine.resetTimeTrackers();
    } else if (g_optDepth > 0) {
        cout << "Engine using think depth " << g_optDepth << endl;
        engine.setThinkDepth(g_optDepth);
    }

    haveScore = haveMove = false;

    while (!g_quitFlag) {
        waitResult = waiter.wait(-1);

        if (waitResult == 0) {
            message = engine.dequeueMessage();

            if (message.get() == 0) {
                cerr << "Failed to read an expected message from engine " << engine.id() << endl;
                return false;
            }

            switch (message->type) {
            case EngineMessage::TYPE_BEST_MOVE: {
                EngineMessageBestMove *engineMessageBestMove = dynamic_cast<EngineMessageBestMove *> (message.get());
                move = engineMessageBestMove->bestMove;
                haveMove = true;
                break;
            }

            case EngineMessage::TYPE_INFO_SEARCH: {
                EngineMessageInfoSearch *engineMessageInfoSearch = dynamic_cast<EngineMessageInfoSearch *>
                                                                   (message.get());

                if (engineMessageInfoSearch->have & EngineMessageInfoSearch::HAVE_MATESCORE) {
                    score = engineMessageInfoSearch->mateScore > 0 ? 30000 : -30000;
                    haveScore = true;
                } else if (engineMessageInfoSearch->have & EngineMessageInfoSearch::HAVE_SCORE) {
                    score = engineMessageInfoSearch->score;
                    haveScore = true;
                }

                displayInfo(message);

                break;
            }
            default: {
                LOGDBG << "Ignoring message " << EngineMessage::typeDesc(message->type) <<
                    " from engine " << engine.id();
                break;
            }
            }
        } else {
            // Quit or error
            return false;
        }

        if (haveMove) {
            if (!haveScore) {
                cerr << "No score from engine" << endl;
                return false;
            }

            break;
        }
    }

    return g_quitFlag ? false : true;
}

//
// Start the engine analysing the position and wait for the answer
//
static bool getBestMove(Engine &engine, const Position &pos, Move &move) {
    shared_ptr<EngineMessage> message;
    TimeTracker whiteTimeTracker(g_optTimeControl), blackTimeTracker(g_optTimeControl);
    ostringstream oss;
    int waitResult;
    bool haveMove;

    IoEventList e(2);
    IoEventWaiter waiter;
    e[0] = &(engine.fromQueue().event());
    e[1] = &g_quitEvent;

    if (!waiter.setEvents(e)) {
        cerr << "Failed to set waiter events";
        return false;
    }

    if (g_optTimeControl.isValid()) {
        cout << "Engine using time control '" << g_optTimeControl.notation(TimeControlPeriod::FORMAT_PGN) << "'" << endl;
        engine.setTimeTrackers(&whiteTimeTracker, &blackTimeTracker);
        engine.resetTimeTrackers();
    } else if (g_optDepth > 0) {
        cout << "Engine using think depth " << g_optDepth << endl;
        engine.setThinkDepth(g_optDepth);
    }

    move.init();

    engine.enqueueMessage(NEW_ENGINE_MESSAGE(TYPE_GO));

    haveMove = false;

    while (!haveMove && !g_quitFlag) {
        waitResult = waiter.wait(-1);

        if (waitResult == 0) {
            message = engine.dequeueMessage();

            if (message.get() == 0) {
                cerr << "Failed to read an expected message from engine " << engine.id() << endl;
                return false;
            }

            switch (message->type) {
            case EngineMessage::TYPE_BEST_MOVE: {
                EngineMessageBestMove *engineMessageBestMove = dynamic_cast<EngineMessageBestMove *> (message.get());
                move = engineMessageBestMove->bestMove;
                haveMove = true;
                break;
            }

            case EngineMessage::TYPE_INFO_SEARCH: {
                displayInfo(message);
                break;
            }

            default: {
                LOGDBG << "Ignoring message " << EngineMessage::typeDesc(message->type) <<
                    " from engine " << engine.id();
                break;
            }
            }
        } else {
            // Quit or error
            return false;
        }
    }

    if (g_quitFlag)
        return false;

    return true;
}

static bool displayInfo(const shared_ptr<EngineMessage> &message) {
    EngineMessageInfoSearch *info = dynamic_cast<EngineMessageInfoSearch *> (message.get());

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
