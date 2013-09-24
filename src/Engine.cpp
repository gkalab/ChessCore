//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Engine.cpp: Engine class implementation.
//

#define VERBOSE_LOGGING 0

#include <ChessCore/Engine.h>
#include <ChessCore/UCIEngineOption.h>
#include <ChessCore/Position.h>
#include <ChessCore/Log.h>

#ifndef WINDOWS
#include <signal.h>
#include <errno.h>
#endif // !WINDOWS
#include <string.h>
#include <sstream>
#include <algorithm>
#include <memory>

using namespace std;

namespace ChessCore {

const char *Engine::m_classname = "Engine";

const char *Engine::m_stateDescs[Engine::NUM_STATES] = {
    "Unloaded",         // STATE_UNLOADED
    "Loaded",           // STATE_LOADED
    "Idle",             // STATE_IDLE
    "Ready",            // STATE_READY
    "Thinking"          // STATE_THINKING
};

const char *Engine::stateDesc(State state) {
    if (state < NUM_STATES)
        return m_stateDescs[state];
    return "Unkown";
}

Engine::Engine() :
    Thread(),
	m_process(),
	m_state(STATE_UNLOADED),
	m_id(),
	m_name(),
	m_author(),
	m_timeout(3000),
	m_engineOptions(),
    m_pendingConfig(),
	m_uciDebugFunc(0),
	m_uciDebugUserp(0),
	m_fromQueue(),
	m_toQueue(),
	m_ioThreadQuitEvent(),
#ifdef WINDOWS
    m_overlapped(),
#endif // WINDOWS
	m_position(),
	m_positionString(),
	m_timeControl(),
    m_thinkingAsWhite(false),
	m_discardNextBestMove(false) {

#ifdef WINDOWS
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif // WINDOWS
}

Engine::~Engine() {
    unload();
    m_state = STATE_UNLOADED;

#ifdef WINDOWS
    ::CloseHandle(m_overlapped.hEvent);
#endif // WINDOWS
}

bool Engine::load(const string &exeFile, const string &workDir, unsigned startupTimeout, unsigned timeout) {

    ASSERT(!exeFile.empty());
    ASSERT(startupTimeout > 0);
    ASSERT(timeout > 0);

    shared_ptr<EngineMessage> message;
    bool first = true, uciok = false;

    if (m_state != STATE_UNLOADED) {
        LOGERR << "Engine " << id() << ": Already loaded";
        return false;
    }

    // Reset anything from previous runs
    m_engineOptions.clear();
    m_pendingConfig.clear();
    m_position.setStarting();
    m_positionString.clear();

    m_timeout = timeout;

    if (!m_process.load(m_id, exeFile, workDir)) {
        LOGERR << "Engine " << id() << ": Failed to load engine process";
        return false;
    }

    m_state = STATE_LOADED;

    // Start the I/O thread
    start();

    // Wait for a TYPE_MAINLOOP_ALIVE message from the I/O thread
    message = m_fromQueue.dequeue(startupTimeout);
    if (message.get() == 0) {
        LOGERR << "Engine " << id() << ": I/O thread failed to start properly";
        unload();
        return false;
    } else if (message->type != EngineMessage::TYPE_MAINLOOP_ALIVE) {
        LOGERR << "Engine " << id() <<
            ": Expected message " << EngineMessage::typeDesc(EngineMessage::TYPE_MAINLOOP_ALIVE) <<
            " but received message " << EngineMessage::typeDesc(message->type);
        unload();
        return false;
    }

    LOGINF << "Engine " << id() << ": Loaded";

    if (!m_toQueue.enqueue(NEW_ENGINE_MESSAGE(TYPE_UCI))) {
        unload();
        return false;
    }

    // Get id and options from engine and most importantly the 'uciok' message
    while (m_threadRunning && !uciok) {
        message = m_fromQueue.dequeue(m_timeout);
        if (message.get() == 0) {
            if (first) {
                LOGERR << "Engine " << id() << ": Timed-out getting message";
                unload();
                return false;
            } else {
                continue;
            }
        }

        switch (message->type) {
        case EngineMessage::TYPE_ID: {
            EngineMessageId *engineMessageId = dynamic_cast<EngineMessageId *> (message.get());

            if (engineMessageId->name == "name")
                m_name = engineMessageId->value;
            else if (engineMessageId->name == "author")
                m_author = engineMessageId->value;
            else
                LOGWRN << "Engine " << id() << ": Unknown UCI id value '" << engineMessageId->name << "'";

            break;
        }

        case EngineMessage::TYPE_UCI_OK: {
            uciok = true;
            break;
        }

        case EngineMessage::TYPE_REGISTRATION_ERROR: {
            LOGINF << "Engine " << id() << ": Engine needs to be registered";
            m_unregistered = true;
            break;
        }

        case EngineMessage::TYPE_OPTION: {
            EngineMessageOption *engineMessageOption = dynamic_cast<EngineMessageOption *> (message.get());
            m_engineOptions[engineMessageOption->option.name()] = engineMessageOption->option;
            break;
        }

        case EngineMessage::TYPE_INFO_STRING: {
            EngineMessageInfoString *engineMessageInfoString = dynamic_cast<EngineMessageInfoString *>
                                                               (message.get());
            LOGINF << "Engine " << id() << ": Engine info message '" << engineMessageInfoString->info << "'";
            break;
        }

        case EngineMessage::TYPE_ERROR: {
            EngineMessageError *engineMessageError = dynamic_cast<EngineMessageError *> (message.get());
            LOGERR << "Engine " << id() << ": Engine error message '" << engineMessageError->error << "'";
            unload();
            return false;
        }

        default: {
            LOGDBG << "Engine " << id() << ": Ignoring unexpected message: " << EngineMessage::typeDesc(
                message->type);
            break;
        }
        }
    }

    if (!m_threadRunning) {
        LOGERR << "Engine " << id() << ": I/O thread terminated";
        unload();
        return false;
    }

    if (!uciok) {
        LOGERR << "Engine " << id() << ": Did not get 'uciok' message";
        unload();
        return false;
    }

    // Wait for the engine to become ready
    if (!isReady()) {
        unload();
        return false;
    }

    m_state = STATE_IDLE;

    LOGINF << "Engine '" << m_name << "' by '" << m_author << "' is ready";

    return true;
}

bool Engine::unload() {
    if (m_process.isLoaded()) {
        LOGDBG << "Engine " << id() << ": Unloading";
    }

    if (m_threadRunning) {
        // Tell the engine to quit
        m_toQueue.enqueue(NEW_ENGINE_MESSAGE(TYPE_QUIT));

        // Stop the I/O thread
        m_ioThreadQuitEvent.set();

        while (m_threadRunning) {
            LOGDBG << "Engine " << id() << ": Waiting for I/O thread to stop";
            Util::sleep(100);
        }
        LOGDBG << "Engine " << id() << ": I/O thread stopped";
    }

    // Unload the process
    m_process.unload();

    m_state = STATE_UNLOADED;

    return true;
}

bool Engine::getReady(int timeout /*=3000*/) {
    if (m_state == STATE_UNLOADED) {
        LOGERR << "Engine " << id() << ": Not loaded";
        return false;
    }

    while (!hasPositionString() && timeout > 0) {
        Util::sleep(250);
        timeout -= 250;
    }

    if (!hasPositionString()) {
        LOGERR << "Engine " << id() << ": No position string set";
        return false;
    }

    if (!writeToEngine(m_positionString)) {
        LOGERR << "Engine " << id() << ": Failed to wring position string";
        return false;
    }

    m_state = STATE_READY;
    return true;
}

bool Engine::startThinking() {
    if (m_state != STATE_READY) {
        LOGERR << "Engine " << id() << ": Not ready";
        return false;
    }

    if (writeToEngine(uciFromEngineMessage(NEW_ENGINE_MESSAGE(TYPE_GO)))) {
        m_state = STATE_THINKING;
        return true;
    }

    return false;
}

bool Engine::stopThinking() {
    if (m_state != STATE_THINKING) {
        LOGERR << "Engine " << id() << ": Not thinking";
        return false;
    }

    if (!writeToEngine("stop"))
        return false;

    m_state = STATE_READY;
    return true;
}

void Engine::resetQueues() {
    m_toQueue.clear();
    m_fromQueue.clear();
    m_ioThreadQuitEvent.reset();
}

#ifdef WINDOWS
// Overlapped I/O completion routine, triggered with entry()
static VOID CALLBACK readComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    const char *m_classname = "";
    LOGVERBOSE << "*** dwErrorCode=" << hex << dwErrorCode << "dwNumberOfBytesTransfered=" << dec << dwNumberOfBytesTransfered;
    if (dwErrorCode == 0) {
        if (!::SetEvent(lpOverlapped->hEvent)) {
            LOGERR << "Failed to set overlapped I/O event: " << Util::win32ErrorText(GetLastError());
        }
    } else if (dwErrorCode != ERROR_BROKEN_PIPE) {
        // ERROR_BROKEN_PIPE happens when the process terminates
        LOGERR << "Failed to read from process: 0x" << hex << dwErrorCode;
    }
}
#endif // WINDOWS

void Engine::entry() {
    enum {
        INDEX_FROM_ENGINE,
        INDEX_FROM_GUI,
        INDEX_QUIT,
        //
        NUM_EVENTS
    };

    try {
        ssize_t numRead;
        int i;
        bool quit = false;
        string toMessage;
        ostringstream fromMessage;
        IoEventWaiter waiter;
        IoEventList e(NUM_EVENTS);
#ifdef WINDOWS
        IoEvent fromEngineEvent(m_overlapped.hEvent);
#else // !WINDOWS
        IoEvent fromEngineEvent(m_process.fromFD());
#endif // WINDOWS
        shared_ptr<EngineMessage> message;
        uint8_t buffer[4096];

#ifdef WINDOWS

        m_doOverlappedRead = true;

#else // !WINDOWS
        // Block all signals (especially important during profiling to avoid being
        // killed by SIGPROF)
        sigset_t sigset;
        sigfillset(&sigset);
        int sigerr = pthread_sigmask(SIG_SETMASK, &sigset, NULL);
        if (sigerr != 0)
        {
            LOGERR << "Failed to block thread signals: " << strerror(sigerr) << " (" << sigerr << ")";
            return;
        }
#endif // WINDOWS

        e[INDEX_FROM_ENGINE] = &fromEngineEvent;
        e[INDEX_FROM_GUI] = &m_toQueue.event();
        e[INDEX_QUIT] = &m_ioThreadQuitEvent;

        if (!waiter.setEvents(e)) {
            LOGERR << "Engine " << id() << ": Failed to set IoEventWaiter events";
            return;
        }

        resetQueues();

        LOGINF << "Engine " << id() << ": I/O thread starting";

        // Let load() know we have started
        m_fromQueue.enqueue(NEW_ENGINE_MESSAGE(TYPE_MAINLOOP_ALIVE));

        while (!quit) {

#ifdef WINDOWS
        // Windows uses asynchronous overlapped I/O, which works like this:
        // 1) A read is initiated in this loop whenever m_doOverlappedRead is true.
        // 2) The completion routine sets the overlapped event which will be noticed by waiter.wait(), below.
        // 3) The read is completed within readFromEngine() which also sets m_doOverlappedRead in
        //    order to get the next piece of data from the engine.
        // A bit of a mess really.
        if (m_doOverlappedRead) {
            LOGVERBOSE << "*** Starting read";
            if (!::ResetEvent(m_overlapped.hEvent)) {
                LOGERR << "Engine " << id() << ": Failed to reset overlapped I/O event: " << Util::win32ErrorText(GetLastError());
                quit = true;
                continue;
            }
            if (!::ReadFileEx(m_process.fromHandle(), buffer, sizeof(buffer), &m_overlapped, readComplete)) {
                LOGERR << "Engine " << id() << ": Failed to initiate overlapped I/O: " << Util::win32ErrorText(GetLastError());
                quit = true;
                continue;
            }
            m_doOverlappedRead = false;
        }
#endif // WINDOWS

            int index = waiter.wait();

            if (index == IO_EVENT_WAIT_ERROR) {
                LOGERR << "Engine " << id() << ": Error waiting for I/O event: " << strerror(
                    errno) << " (" << errno << ")";
                quit = true;
            } else if (index == IO_EVENT_WAIT_HANGUP) {
                LOGINF << "Engine " << id() << ": Hang-up detected waiting for I/O event";
                m_fromQueue.enqueue(NEW_ENGINE_MESSAGE_ERROR("Engine has terminated"));
                quit = true;
            } else if (index == INDEX_FROM_GUI) {
                message = m_toQueue.dequeue();

                if (message.get() != 0) {
                    LOGVERBOSE << "Engine " << id() << ": Message from GUI: " << EngineMessage::typeDesc(message->type);

                    bool valid = false;
                    State newState = m_state;

                    switch (message->type) {
                    case EngineMessage::TYPE_UCI:
                        valid = m_state == STATE_LOADED;
                        newState = STATE_IDLE;
                        break;

                    case EngineMessage::TYPE_DEBUG:
                    case EngineMessage::TYPE_IS_READY:
                    case EngineMessage::TYPE_NEW_GAME:
                        valid = m_state != STATE_UNLOADED && m_state != STATE_THINKING;
                        break;

                    case EngineMessage::TYPE_SET_OPTION:
                        valid = m_state != STATE_UNLOADED;
                        break;

                    case EngineMessage::TYPE_POSITION:
                        valid = m_state > STATE_UNLOADED;
                        newState = STATE_READY;
                        break;

                    case EngineMessage::TYPE_GO:
                        valid = m_state == STATE_READY;
                        newState = STATE_THINKING;
                        break;

                    case EngineMessage::TYPE_STOP:
                        valid = m_state == STATE_THINKING;
                        newState = STATE_READY;
                        break;

                    case EngineMessage::TYPE_PONDER_HIT:
                        valid = m_state == STATE_THINKING;
                        break;

                    case EngineMessage::TYPE_QUIT:
                        valid = m_state != STATE_UNLOADED;
                        newState = STATE_UNLOADED;
                        break;

                    default:
                        valid = false;
                        break;
                    }

                    if (valid) {
                        if (message->type == EngineMessage::TYPE_POSITION) {
                            // Handle this specially
                            EngineMessagePosition *engineMessagePosition = dynamic_cast<EngineMessagePosition *>(message.get());

                            State oldState = m_state;

                            if (oldState == STATE_THINKING) {
                                m_discardNextBestMove = true;

                                // stopThinking() sets state
                                quit = !stopThinking();
                            }

                            toMessage = uciFromEngineMessage(message);

                            if (!toMessage.empty()) {
                                quit = !writeToEngine(toMessage);
                                m_state = newState;

                                // Save the current position for processing any moves generated
                                // and the UCI position for restarting the engine if we need to later
                                m_position = engineMessagePosition->currentPosition;
                                m_positionString = toMessage;
                            }

                            // Restart the engine thinking
                            if (oldState == STATE_THINKING)
                                // startThinking sets state
                                quit = !startThinking();
                        } else if (message->type == EngineMessage::TYPE_SET_OPTION &&
                                   m_state == STATE_THINKING) {
                            // We won't disturb the engine now; we'll wait until it's finished
                            m_pendingConfig.push_back(message);
                        } else {
                            toMessage = uciFromEngineMessage(message);
                            if (!toMessage.empty()) {
                                if (message->type == EngineMessage::TYPE_GO) {
                                    // Record which side the engine is thinking as and the time
                                    // it started thinking
                                    m_thinkingAsWhite = (toColour(m_position.ply()) == BLACK);
                                    m_timeControl.startTime = Util::getTickCount();
                                }

                                quit = !writeToEngine(toMessage);
                            }

                            if (!quit)
                                m_state = newState;
                        }
                    } else {
                        sendErrorToGUI("Engine %s: Cannot process message '%s' when in state '%s'",
                                       id().c_str(), EngineMessage::typeDesc(message->type), stateDesc(m_state));
                    }
                } else {
                    LOGERR << "Engine " << id() << ": Failed to get message from 'toQueue'";
                    quit = true;
                }
            } else if (index == INDEX_FROM_ENGINE) {
                numRead = readFromEngine(buffer, sizeof(buffer));
                if (numRead < 0) {
                    quit = true;
                } else {
#if VERBOSE_LOGGING && 0
                    LOGVERBOSE << "Engine " << id() << ":";
                    LOGVERBOSE << Util::formatData(buffer, numRead);
#endif // VERBOSE_LOGGING

                    for (i = 0; i < numRead; i++) {
                        uint8_t c = buffer[i];

                        if (c == '\r' || c == '\n') {
                            if (fromMessage.tellp() > 0) {
                                string uci = fromMessage.str();

                                shared_ptr<EngineMessage> message = engineMessageFromUCI(uci);
                                if (message.get() != 0) {
                                    if (message->type == EngineMessage::TYPE_BEST_MOVE) {
                                        if (!m_discardNextBestMove) {
                                            m_state = STATE_READY;
                                            m_timeControl.endTime = Util::getTickCount();
                                            m_timeControl.elapsed = m_timeControl.endTime - m_timeControl.startTime;

                                            if (m_thinkingAsWhite && m_timeControl.whiteTime > 0)
                                                m_timeControl.whiteTime -= m_timeControl.elapsed;
                                            else if (!m_thinkingAsWhite && m_timeControl.blackTime > 0)
                                                m_timeControl.blackTime -= m_timeControl.elapsed;
                                            else if (m_timeControl.moveTime > 0)
                                                m_timeControl.moveTime -= m_timeControl.elapsed;
                                        } else {
                                            m_discardNextBestMove = false;
                                        }

                                        // Send any pending configuration now the engine has finished thinking
                                        quit = !sendPendingConfig();
                                    }

                                    // Send engine message to the UI
                                    m_fromQueue.enqueue(message);
                                }

                                if (m_uciDebugFunc != 0)
                                    (m_uciDebugFunc)(m_uciDebugUserp, this, true, uci);

                                fromMessage.str("");
                            }
                        } else {
                            fromMessage << c;
                        }
                    }
                }
            } else if (index == INDEX_QUIT) {
                quit = true;
            }
        }
    } catch(ChessCoreException &e) {
        LOGERR << "Engine " << id() << ": ChessCoreException in Engine I/O thread: " << e.what();
    } catch(exception &e) {
        LOGERR << "Engine " << id() << ": std::exception in Engine I/O thread: " << e.what();
    } catch(...) {
        LOGERR << "Engine " << id() << ": Unknown exception in Engine I/O thread";
    }

    LOGINF << "Engine " << id() << ": I/O thread stopped";
}

bool Engine::isReady() {

	if (!m_toQueue.enqueue(NEW_ENGINE_MESSAGE(TYPE_IS_READY)))
        return false;

    shared_ptr<EngineMessage> message;

    while (true) {
        // All the time we are getting something back from the engine, then
        // keep the timeout value the same.
        message = m_fromQueue.dequeue(m_timeout);

        if (message.get() == 0) {
            LOGERR << "Engine " << id() << ": Did not become ready in time";
            return false;
        }

		if (message->type == EngineMessage::TYPE_READY_OK) {
            break;
		} else {
            LOGWRN << "Engine " << id() << ": Ignoring unknown response '" << EngineMessage::typeDesc(message->type) << "'";
		}
    }

    return true;
}

string Engine::uciFromEngineMessage(const shared_ptr<EngineMessage> engineMessage) {
    string uci;

    switch (engineMessage->type) {
    case EngineMessage::TYPE_UCI: {
        uci = "uci";
        break;
    }

    case EngineMessage::TYPE_DEBUG: {
        const EngineMessageDebug *engineMessageDebug = dynamic_cast<const EngineMessageDebug *>
                                                       (engineMessage.get());
        uci = Util::format("debug %s", engineMessageDebug->debug ? "on" : "off");
        break;
    }

    case EngineMessage::TYPE_IS_READY: {
        uci = "isready";
        break;
    }

    case EngineMessage::TYPE_REGISTER: {
        const EngineMessageRegister *engineMessageRegister = dynamic_cast<const EngineMessageRegister *>
                                                             (engineMessage.get());

        if (!engineMessageRegister->name.empty()) {
            if (!engineMessageRegister->code.empty())
                uci = Util::format("register name %s code %s", engineMessageRegister->name.c_str(),
                                   engineMessageRegister->code.c_str());
            else
                uci = Util::format("register name %s", engineMessageRegister->name.c_str());
        } else if (engineMessageRegister->later)
            uci = "register later";

        break;
    }

    case EngineMessage::TYPE_SET_OPTION: {
        const EngineMessageSetOption *engineMessageSetOption = dynamic_cast<const EngineMessageSetOption *>
                                                               (engineMessage.get());
        uci = uciForSetOption(engineMessageSetOption->name, engineMessageSetOption->value);
        break;
    }

    case EngineMessage::TYPE_NEW_GAME: {
        uci = "ucinewgame";
        break;
    }

    case EngineMessage::TYPE_POSITION: {
        const EngineMessagePosition *engineMessagePosition = dynamic_cast<const EngineMessagePosition *>
                                                             (engineMessage.get());
        ostringstream oss;

        if (engineMessagePosition->startPosition.hashKey() == 0ULL &&
            engineMessagePosition->moves.empty())
            oss << "position fen " << engineMessagePosition->currentPosition.fen();
        else {
            oss << "position ";

            if (engineMessagePosition->startPosition.isStarting())
                oss << "startpos";
            else
                oss << "fen " << engineMessagePosition->startPosition.fen();

            if (!engineMessagePosition->moves.empty())
                oss << " moves";

            for (auto it = engineMessagePosition->moves.begin(); it != engineMessagePosition->moves.end();
                 ++it) {
                Move move = *it;
                oss << " " << move.coord(true);
            }
        }

        uci = oss.str();
        break;
    }

    case EngineMessage::TYPE_GO: {
        if (m_timeControl.infinite)
            uci = "go infinite";
        else if (m_timeControl.whiteTime > 0 && m_timeControl.blackTime > 0)
            uci = Util::format("go wtime %u btime %u winc %u binc %u",
                               m_timeControl.whiteTime, m_timeControl.blackTime,
                               m_timeControl.whiteInc, m_timeControl.blackInc);
        else if (m_timeControl.depth > 0)
            uci = Util::format("go depth %u", m_timeControl.depth);
        else if (m_timeControl.moveTime > 0)
            uci = Util::format("go movetime %u", m_timeControl.moveTime);
        else
            LOGWRN << "Engine " << id() << ": No viable time control value to use";

        break;
    }

    case EngineMessage::TYPE_STOP: {
        uci = "stop";
        break;
    }

    case EngineMessage::TYPE_PONDER_HIT: {
        uci = "ponderhit";
        break;
    }

    case EngineMessage::TYPE_QUIT: {
        uci = "quit";
        break;
    }

    default:
        LOGERR << "Engine " << id() << ": No viable UCI command for engine command " <<
            EngineMessage::typeDesc(engineMessage->type);
        break;
    }

    if (uci.empty())
        LOGWRN << "Engine " << id() << ": Failed to convert engine command " <<
            EngineMessage::typeDesc(engineMessage->type) << " to UCI";

    return uci;
}

string Engine::uciForSetOption(const string &name, const string &value) {
    string uci;
    StringOptionMap::iterator it = m_engineOptions.find(name);

    if (it != m_engineOptions.end()) {
        UCIEngineOption &uciEngineOption = it->second;

        if (value.length() > 0 &&
            uciEngineOption.type() != UCIEngineOption::TYPE_BUTTON)
            uci = Util::format("setoption name %s value %s", name.c_str(), value.c_str());
        else if (uciEngineOption.type() == UCIEngineOption::TYPE_BUTTON)
            uci = Util::format("setoption name %s", name.c_str());
        else
            LOGERR << "Engine " << id() << ": Failed to parse engine message option '" <<
                name << "' due to value/type incompatibility";
    } else {
        LOGERR << "Engine " << id() << ": Option '" << name << "' is not supported";
    }

    return uci;
}

shared_ptr<EngineMessage> Engine::engineMessageFromUCI(const string &uci) {
    string message;

    vector<string> parts;
    unsigned numParts;
    UCIEngineOption option;
    ostringstream pvStr;

    numParts = Util::splitLine(uci, parts);

    if (numParts == 0)
        return 0;

    if (parts[0] == "id") {
        if (numParts >= 3)
            return NEW_ENGINE_MESSAGE_ID(parts[1], Util::concat(parts, 2, numParts));
    } else if (parts[0] == "uciok")
        return NEW_ENGINE_MESSAGE(TYPE_UCI_OK);
    else if (parts[0] == "registration") {
        if (numParts == 2)
            if (parts[1] == "error")
                return NEW_ENGINE_MESSAGE(TYPE_REGISTRATION_ERROR);

    } else if (parts[0] == "readyok")
        return NEW_ENGINE_MESSAGE(TYPE_READY_OK);
    else if (parts[0] == "bestmove") {
        // We not only parse the moves, but also make the moves in the current position in order
        // to get the position.lastMove() which contains additional move flags (check/mate).
        Move bestMove, ponderMove;

        if (numParts >= 2) {
            if (bestMove.parse(m_position, parts[1])) {
                Position nextPos = m_position;
                UnmakeMoveInfo umi;

                if (nextPos.makeMove(bestMove, umi)) {
                    bestMove = nextPos.lastMove();

                    if (numParts >= 4 && parts[2] == "ponder" && parts[3] != "(none)") {
                        if (ponderMove.parse(nextPos, parts[3])) {
                            if (nextPos.makeMove(ponderMove, umi))
                                ponderMove = nextPos.lastMove();
                            else
                                LOGWRN << "Engine " << id() << ": Failed to make ponder move '" << ponderMove << "'";
                        } else {
                            LOGWRN << "Engine " << id() << ": Failed to parse ponder move '" << parts[3] << "'";
                        }
                    }
                } else {
                    return NEW_ENGINE_MESSAGE_ERROR(Util::format("Failed to make best move %s from engine %s",
                                                                 bestMove.dump().c_str(), id().c_str()));
                }
            } else {
                return NEW_ENGINE_MESSAGE_ERROR(Util::format("Failed to parse best move '%s' from engine %s",
                                                             parts[1].c_str(), id().c_str()));
            }
        }

        return shared_ptr<EngineMessage> (new EngineMessageBestMove(bestMove, ponderMove));
    } else if (parts[0] == "info") {
        unsigned numParts = (unsigned)(parts.size());
        unsigned i = 1;

        if (numParts > 2 && parts[1] == "string")
            return NEW_ENGINE_MESSAGE_INFO_STRING(Util::concat(parts, 2, (unsigned)parts.size()));

        shared_ptr<EngineMessageInfoSearch> info(new EngineMessageInfoSearch());

        while (i < numParts) {
            if (parts[i] == "depth") {
                if (i + 1 < numParts) {
                    if (!Util::parse(parts[i + 1], info->depth)) {
                        LOGERR << "Engine " << id() << ": Failed to parse info depth value '" << parts[i + 1] << "'";
                        return 0;
                    }

                    i += 2;
                    info->have |= EngineMessageInfoSearch::HAVE_DEPTH;
                } else {
                    LOGWRN << "Engine " << id() << ": No depth value specified";
                    break;
                }
            } else if (parts[i] == "seldepth") {
                if (i + 1 < numParts) {
                    if (!Util::parse(parts[i + 1], info->selectiveDepth)) {
                        LOGERR << "Engine " << id() << ": Failed to parse info seldepth value '" << parts[i + 1] << "'";
                        return 0;
                    }

                    i += 2;
                    info->have |= EngineMessageInfoSearch::HAVE_SELDEPTH;
                } else {
                    LOGWRN << "Engine " << id() << ": No seldepth value specified";
                    break;
                }
            } else if (parts[i] == "pv") {
                i++;
                info->pv.reserve(numParts - 1);

                Position posTemp = m_position, posTemp2;
                UnmakeMoveInfo umi;
                Move move;

                while (i < numParts) {
                    if (!move.parse(posTemp, parts[i])) {
                        // Assumes the 'pv' takes the rest of the line...
                        LOGWRN << "Engine " << id() << ": Failed to parse info pv move '" << parts[i] << "'";
                        break; // Ignore
                    }

                    posTemp2.set(posTemp);

                    if (!posTemp.makeMove(move, umi)) {
                        LOGWRN << "Engine " << id() << ": Failed to make info pv move '" << move << "'";
                        break; // Ignore
                    }

                    // Use the lastMove made in the position as check or double check flags might
                    // have been set
                    info->pv.push_back(posTemp.lastMove());

                    if (pvStr.tellp() > 0)
                        pvStr << ' ';

                    pvStr << posTemp.lastMove().san(posTemp2);
                    i++;
                }

                if (info->pv.size() > 0) {
                    info->have |= EngineMessageInfoSearch::HAVE_PV;
                    info->pvStr = pvStr.str();
                }
            } else if (parts[i] == "score") {
                i++;

                if (i < numParts) {
                    if (parts[i] == "cp") {
                        if (!Util::parse(parts[i + 1], info->score)) {
                            LOGERR << "Engine " << id() << ": Failed to parse info score value '" <<
                                parts[i + 1] << "'";
                            return 0;
                        }

                        // Normalize the score to be from white's point-of-view
                        if (toColour(m_position.ply()) == WHITE)
                            info->score = -info->score;

                        i += 2;
                        info->have |= EngineMessageInfoSearch::HAVE_SCORE;
                    } else if (parts[i] == "mate") {
                        if (!Util::parse(parts[i + 1], info->mateScore)) {
                            LOGERR << "Engine " << id() << ": Failed to parse info mate value '" << parts[i + 1] << "'";
                            return 0;
                        }

                        // Normalize the score to be from white's point-of-view
                        if (toColour(m_position.ply()) == WHITE)
                            info->mateScore = -info->mateScore;

                        i += 2;
                        info->have |= EngineMessageInfoSearch::HAVE_MATESCORE;
                    }

                    // else ignore lowerbound and upperbound
                } else {
                    LOGWRN << "Engine " << id() << ": No score value specified";
                    break;
                }
            } else if (parts[i] == "time") {
                if (i + 1 < numParts) {
                    if (!Util::parse(parts[i + 1], info->time)) {
                        LOGERR << "Engine " << id() << ": Failed to parse info time value '" << parts[i + 1] << "'";
                        return 0;
                    }

                    i += 2;
                    info->have |= EngineMessageInfoSearch::HAVE_TIME;
                } else {
                    LOGWRN << "Engine " << id() << ": No time value specified";
                    break;
                }
            } else if (parts[i] == "nodes") {
                if (i + 1 < numParts) {
                    if (!Util::parse(parts[i + 1], info->nodes)) {
                        LOGERR << "Engine " << id() << ": Failed to parse info nodes value '" << parts[i + 1] << "'";
                        return 0;
                    }

                    i += 2;
                    info->have |= EngineMessageInfoSearch::HAVE_NODES;
                } else {
                    LOGWRN << "Engine " << id() << ": No nodes value specified";
                    break;
                }
            } else if (parts[i] == "nps") {
                if (i + 1 < numParts) {
                    if (!Util::parse(parts[i + 1], info->nps)) {
                        LOGERR << "Engine " << id() << ": Failed to parse info nps value '" << parts[i + 1] << "'";
                        return 0;
                    }

                    i += 2;
                    info->have |= EngineMessageInfoSearch::HAVE_NPS;
                } else {
                    LOGWRN << "Engine " << id() << ": No nps value specified";
                    break;
                }
            } else if (parts[i] == "multipv" || parts[i] == "currmove" || parts[i] == "currmovenum" ||
                       parts[i] == "hashfull" || parts[i] == "tbhits" || parts[i] == "sbhits" || parts[i] == "cpuload") {
                i += 2; // Ignore
            } else if (parts[i] == "string" || parts[i] == "refutation" || parts[i] == "currline") {
                break; // Ignore rest of the line
            } else {
                break;
            }
        }

        return info;
    } else if (parts[0] == "option") {
        option.clear();

        if (!option.set(parts)) {
            LOGERR << "Engine " << id() << ": Failed to parse option";
            return 0;
        } else if (!option.isValid()) {
                LOGWRN << "Engine " << id() << " declared an invalid option '" << option.dump() << "' (ignored)";
        } else {
            if (option.name() != "UCI_Chess960") {
                LOGVERBOSE << "Engine " << id() << " supports option '" << option.dump() << "'";
                return NEW_ENGINE_MESSAGE_OPTION(option);
            } else {
                LOGWRN << "Engine " << id() << ": Ignoring support for option '" << option.dump() << "'";
                return 0;
            }
        }
    }

    // Put whatever else is generated into an info string message
    return NEW_ENGINE_MESSAGE_INFO_STRING(uci);
}

ssize_t Engine::readFromEngine(uint8_t *buffer, unsigned buflen) {

#ifdef WINDOWS

    DWORD numRead = 0;
    if (!::GetOverlappedResult(m_process.fromHandle(), &m_overlapped, &numRead, FALSE)) {
        LOGERR << "Engine " << id() << ": Failed to read UCI message: " << Util::win32ErrorText(GetLastError());
        return -1;
    }
    if (VERBOSE_LOGGING) {
        LOGVERBOSE << "Engine " << id() << ": Read " << numRead << " bytes: ";
        LOGVERBOSE << Util::formatData(buffer, numRead);
    }
    m_doOverlappedRead = true;
    return (ssize_t)numRead;

#else // !WINDOWS

    ssize_t numRead = ::read(m_process.fromFD(), buffer, sizeof(buffer));
    if (numRead < 0) {
        LOGERR << "Engine " << id() << ": failed to read UCI message: " << strerror(errno) << " (" << errno << ")";
    }
    return numRead;

#endif // WINDOWS
}

bool Engine::writeToEngine(const string &message) {
    if (!m_process.isLoaded()) {
        LOGWRN << "Engine " << id() << ": Not loaded";
        return false;
    }

    bool retval = true;

    if (m_uciDebugFunc != 0)
        (m_uciDebugFunc)(m_uciDebugUserp, this, false, message);

    LOGVERBOSE << "Engine " << id() << ": writing '" << message << "'";

    // Commands must end with newline
    string line = message + "\n";

#ifdef WINDOWS

    DWORD numWritten = 0;
    if (!::WriteFile(m_process.toHandle(), line.c_str(), (DWORD)line.length(), &numWritten, 0))
    {
        LOGERR << "Engine " << id() << ": Failed to write UCI command: " << Util::win32ErrorText(GetLastError());
        retval = false;
    }

    LOGVERBOSE << "Engine " << id() << " wrote " << numWritten << " bytes";

#else // !WINDOWS

    if (::write(m_process.toFD(), line.c_str(), line.length()) != (ssize_t)line.length()) {
        LOGERR << "Engine " << id() << ": Failed to write UCI command: " << strerror(
            errno) << " (" << errno << ")";
        retval = false;
    }

#endif // WINDOWS

    return retval;
}

bool Engine::sendErrorToGUI(const char *fmt, ...) {
    char buffer[4096];
    va_list va;

    va_start(va, fmt);
    vsprintf(buffer, fmt, va);
    va_end(va);

    return m_fromQueue.enqueue(NEW_ENGINE_MESSAGE_ERROR(buffer));
}

void Engine::setUciDebug(ENGINE_UCI_DEBUG debugFunc, void *userp) {
    m_uciDebugFunc = debugFunc;
    m_uciDebugUserp = userp;
}

bool Engine::sendPendingConfig() {
    if (m_pendingConfig.empty())
        return true;

    bool retval = true;

    for (auto it = m_pendingConfig.begin(); it != m_pendingConfig.end(); ++it) {
        shared_ptr<EngineMessage> message = *it;
        string uci = uciFromEngineMessage(message);

        if (!uci.empty()) {
            if (!writeToEngine(uci))
                retval = false;
        } else {
            retval = false;
        }
    }

    m_pendingConfig.clear();

    return retval;
}
}   // namespace ChessCore
