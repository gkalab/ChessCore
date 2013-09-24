//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Engine.h: Engine class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Thread.h>
#include <ChessCore/Process.h>
#include <ChessCore/Move.h>
#include <ChessCore/EngineMessage.h>
#include <ChessCore/EngineMessageQueue.h>
#include <ChessCore/UCIEngineOption.h>
#include <ChessCore/Util.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ChessCore {
class Engine;

/**
 * Engine UCI debug callback.  This function will be called whenever a UCI message is
 * sent to the engine or received from the engine.
 *
 * @param engine The pointer to the engine object sending the callback.
 * @param fromEngine true if the message was received from the engine, else false if
 * the message was sent to the engine.
 * @param message The UCI message sent or received.
 */
extern "C"
{
typedef void (*ENGINE_UCI_DEBUG)(void *userp, const Engine *engine, bool fromEngine, const std::string &message);
}

typedef std::unordered_map<std::string, UCIEngineOption> StringOptionMap;

struct CHESSCORE_EXPORT EngineTimeControl {
    unsigned startTime;
    unsigned endTime;
    unsigned elapsed;
    int whiteTime;
    int blackTime;
    int whiteInc;
    int blackInc;
    int moveTime;
    int depth;
    bool infinite;

    EngineTimeControl() :
		startTime(0),
		endTime(0),
		elapsed(0),
		whiteTime(0),
		blackTime(0),
		whiteInc(0),
		blackInc(0),
        moveTime(0),
		depth(0),
		infinite(false) {
    }

    EngineTimeControl &operator=(const EngineTimeControl &other) {
        startTime = other.startTime;
        endTime = other.endTime;
        elapsed = other.elapsed;
        whiteTime = other.whiteTime;
        blackTime = other.blackTime;
        whiteInc = other.whiteInc;
        blackInc = other.blackInc;
        moveTime = other.moveTime;
        depth = other.depth;
        infinite = other.infinite;
        return *this;
    }

    void clear() {
        whiteTime = 0;
        blackTime = 0;
        elapsed = 0;
        whiteInc = 0;
        blackInc = 0;
        moveTime = 0;
        depth = 0;
        infinite = 0;
    }
};

class CHESSCORE_EXPORT Engine : public Thread {
private:
    static const char *m_classname;

public:
    enum State {
        STATE_UNLOADED,      // Engine not loaded
        STATE_LOADED,        // Engine loaded and confirmed as UCI
        STATE_IDLE,          // Engine doing nothing
        STATE_READY,         // Engine has position and ready to start thinking
        STATE_THINKING,      // Engine is thinking about moves
        //
        NUM_STATES
    };

protected:
    static const char *m_stateDescs[NUM_STATES];
    Process m_process;              // Engine process
    State m_state;                  // Engine state
    std::string m_id;               // Engine ID
    std::string m_name;             // Engine name
    std::string m_author;           // Engine author
    bool m_unregistered;            // Engine is unregistered
    unsigned m_timeout;             // Time to wait for commands to complete
    StringOptionMap m_engineOptions;// Options supported by the engine
    std::vector<std::shared_ptr<EngineMessage>> m_pendingConfig; // Configuration waiting to be set
    ENGINE_UCI_DEBUG m_uciDebugFunc;// Callback for UCI sent to/from engine
    void *m_uciDebugUserp;          // Context pointer for m_uciDebugFunc
    EngineMessageQueue m_fromQueue; // Queue of messages from the engine
    EngineMessageQueue m_toQueue;   // Queue of messages to the engine
    IoEvent m_ioThreadQuitEvent;    // Event to quit the I/O thread
#ifdef WINDOWS
    OVERLAPPED m_overlapped;        // Overlapped I/O structure
    bool m_doOverlappedRead;        // Perform overlapped read flag
#endif // WINDOWS
    Position m_position;            // Last position sent to the engine
    std::string m_positionString;   // Last position string sent to the engine
    EngineTimeControl m_timeControl; // Time controls
    bool m_thinkingAsWhite;         // True then engine thinking as white, else thinking as black
    bool m_discardNextBestMove;     // True if we will discard the next 'bestmove' from the engine

public:
    /**
     * @return A description of the specified state, or "Unknown".
     */
    static const char *stateDesc(State state);

    Engine();

    virtual ~Engine(void);

    bool operator==(const Engine &other) {
        return this == &other;
    }

    State state() const {
        return m_state;
    }

    bool descreaseState() {
        if (m_state > STATE_UNLOADED) {
            m_state = (State)((int)m_state - 1);
            return true;
        }

        return false;
    }

    /**
     * Load the engine.
     *
     * @param exeFile the path to the engine executable.
     * @param workDir the working directory of the engine.  If this is empty then
     * the working directory is not changed.
     * @param startupTimeout the time to give the engine process to settle down before
     * attempting to talk to it using UCI.  (milliseconds).
     * @param timeout the time to wait for UCI commands to be performed by the engine.
     * (milliseconds).
     *
     * @return true if the engine loaded successfully, else false.  If this
     * returns successfully the engine will be in STATE_IDLE and the background
     * thread will be running.  In order to shutdown the engine you need to
     * signal the quitEvent.
     */
    bool load(const std::string &exeFile, const std::string &workDir, unsigned startupTimeout, unsigned timeout);

    /**
     * Unload the engine.
     *
     * @return true if the engine was unloaded successfully.
     */
    bool unload();

    /**
     * Set the engine process priority.
     *
     * @param background If true, the engine is set to "background priority", else
     * if false, the engine is set to "foreground priority".
     *
     * @return true if the engine priority was successfully changed.
     */
    bool setBackgroundPriority(bool background) {
        return m_process.setBackgroundPriority(background);
    }

    /**
     * Go to ready state.  The engine must be loaded or idle state and have a valid
     * position string.
     *
     * @param timeout The amount of time to wait for the position to arrive from the UI
     * before giving up.
     *
     * @return true if the engine was successfully put into ready state, else false.
     */
    bool getReady(int timeout = 3000);

    /**
     * Start thinking.  The engine must be in ready state and have a valid time control
     * set.
     *
     * @return true if the engine was successfully started thinking.
     */
    bool startThinking();

    /**
     * Stop thinking.  The engine must be in thinking state.  The time control will be
     * updated with the time spent thinking.
     *
     * @return true if the engine successfully stopped thinking.
     */
    bool stopThinking();

    inline const std::string &id() const {
        return m_id;
    }

    inline void setId(const std::string &id) {
        m_id = id;
    }

    inline const std::string &name() const {
        return m_name;
    }

    inline const std::string &author() const {
        return m_author;
    }

    inline bool isUnregistered() const {
        return m_unregistered;
    }

    inline ENGINE_UCI_DEBUG uciDebugFunc() const {
        return m_uciDebugFunc;
    }

    inline void *uciDebugUserp() const {
        return m_uciDebugUserp;
    }

    void setUciDebug(ENGINE_UCI_DEBUG debugFunc, void *userp);

    inline int timeout() const {
        return m_timeout;
    }

    inline void setTimeout(int timeout) {
        m_timeout = timeout;
    }

    inline bool isLoaded() const {
        return m_process.isLoaded();
    }

    EngineMessageQueue &toQueue() {
        return m_toQueue;
    }

    bool enqueueMessage(std::shared_ptr<EngineMessage> engineMessage) {
        return m_toQueue.enqueue(engineMessage);
    }

    EngineMessageQueue &fromQueue() {
        return m_fromQueue;
    }

    std::shared_ptr<EngineMessage> dequeueMessage() {
        return m_fromQueue.dequeue();
    }

    std::shared_ptr<EngineMessage> dequeueMessage(int timeout) {
        return m_fromQueue.dequeue(timeout);
    }

    /**
     * @return A map of UCI options the engine supports.
     */
    const StringOptionMap &engineOptions() const {
        return m_engineOptions;
    }

    bool hasPositionString() const {
        return !m_positionString.empty();
    }

    EngineTimeControl &timeControl() {
        return m_timeControl;
    }

    void setTimeControl(const EngineTimeControl timeControl) {
        m_timeControl = timeControl;
    }

    bool thinkingAsWhite() const {
        return m_thinkingAsWhite;
    }

    /**
     * Reset the to/from queues and the quit event.
     */
    void resetQueues();

protected:
    /**
     * Main I/O engine loop.
     */
    void entry();

    /**
     * Check if the engine is ready by sending an 'isready' command and
     * waiting for a 'readyok' response.
     *
     * @return true if the engine is ready.
     */
    bool isReady();

    /**
     * Convert EngineMessage into UCI commands for the engine.
     *
     * @param engineMessage The EngineMessage.
     *
     * @return The UCI commands. An empty string is returned if an error occurred
     * interpreting the engine message.
     */
    std::string uciFromEngineMessage(const std::shared_ptr<EngineMessage> engineMessage);

    /**
     * Generate the UCI engine command for setting an option.
     *
     * @param name The option name.
     * @param value The option value.  This can be empty.
     *
     * @return The UCI command.  An empty string is returned if an error occurred.
     */
    std::string uciForSetOption(const std::string &name, const std::string &value);

    /**
     * Convert UCI engine output into an EngineMessage object.
     *
     * @param uci UCI engine output.
     *
     * @return The EngineMessage or 0 if an error occurred interpreting the UCI text.
     */
    std::shared_ptr<EngineMessage> engineMessageFromUCI(const std::string &uci);

    /**
     * Read a message from the engine.
     *
     * @param buffer Where to store the message.
     * @param buflen The size of the buffer.
     *
     * @return int The number of bytes written to buffer, or -1 if an error occurred.
     */
    ssize_t readFromEngine(uint8_t *buffer, unsigned buflen);

    /**
     * Write a message to the engine.
     *
     * @param message The message to send.
     *
     * @return true if the message was sent successfully, else false.
     */
    bool writeToEngine(const std::string &message);

    /**
     * Send a EngineMessageError message back to the GUI.
     *
     * @param fmt printf-style formatting string, followed by optional arguments.
     *
     * @return true if the error message was sent successfully.
     */
    bool sendErrorToGUI(const char *fmt, ...);

    /**
     * Send any pending configuration.
     */
    bool sendPendingConfig();
};
} // namespace ChessCore
