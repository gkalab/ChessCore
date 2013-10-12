//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// EngineMessage.h: Engine message class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Move.h>
#include <ChessCore/Position.h>
#include <ChessCore/UCIEngineOption.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace ChessCore {
typedef std::unordered_map<std::string, std::string>   StringStringMap;

class CHESSCORE_EXPORT EngineMessage {
public:
    enum Type {
        TYPE_NONE,

        // GUI -> Engine
        TYPE_UCI,               // EngineMessage
        TYPE_DEBUG,             // EngineMessageDebug
        TYPE_IS_READY,          // EngineMessage
        TYPE_REGISTER,          // EngineMessageRegister
        TYPE_SET_OPTION,        // EngineMessageSetOption
        TYPE_NEW_GAME,          // EngineMessage
        TYPE_POSITION,          // EngineMessagePosition
        TYPE_GO,                // EngineMessage
        TYPE_STOP,              // EngineMessage
        TYPE_PONDER_HIT,        // EngineMessage
        TYPE_QUIT,              // EngineMessage

        // Engine -> GUI
        TYPE_ID,                // EngineMessageId
        TYPE_UCI_OK,            // EngineMessage
        TYPE_REGISTRATION_ERROR, // EngineMessage
        TYPE_READY_OK,          // EngineMessage
        TYPE_BEST_MOVE,         // EngineMessageBestMove
        TYPE_INFO_SEARCH,       // EngineMessageInfoSearch
        TYPE_INFO_STRING,       // EngineMessageInfoString
        TYPE_OPTION,            // EngineMessageOption

        // Internal. Signals to Engine::load() that the mainloop() has started
        TYPE_MAINLOOP_ALIVE,    // EngineMessage

        // GUI -> Engine. Custom UCI commands (mainly for chimp only)
        TYPE_CUSTOM,            // EngineMessageCustom

        // Engine class  -> GUI. Signals an error to the GUI.
        TYPE_ERROR,             // EngineMessageError

        // Number of types
        NUM_TYPES
    };

protected:
    static const char *typeDescs[NUM_TYPES];

public:
    Type type;

    static const char *typeDesc(Type type);

    EngineMessage(Type type):type(type) {
    }

    virtual ~EngineMessage() {
    }
};

class CHESSCORE_EXPORT EngineMessageDebug : public EngineMessage {
public:
    bool debug;

    EngineMessageDebug(bool debug):EngineMessage(TYPE_DEBUG), debug(debug) {
    }

    virtual ~EngineMessageDebug() {
    }
};

class CHESSCORE_EXPORT EngineMessageRegister : public EngineMessage {
public:
    std::string name;
    std::string code;
    bool later;

    EngineMessageRegister(const std::string &name,
                          const std::string &code):EngineMessage(TYPE_REGISTER), name(name), code(
            code), later(false) {
    }

    EngineMessageRegister(bool later):EngineMessage(TYPE_REGISTER), name(), code(), later(later) {
    }

    virtual ~EngineMessageRegister() {
    }
};

class CHESSCORE_EXPORT EngineMessageSetOption : public EngineMessage {
public:
    std::string name;
    std::string value;

    EngineMessageSetOption(const std::string &name,
                           const std::string &value):EngineMessage(TYPE_SET_OPTION), name(name), value(value) {
    }

    virtual ~EngineMessageSetOption() {
    }
};

class CHESSCORE_EXPORT EngineMessagePosition : public EngineMessage {
public:
    Position currentPosition;
    Position startPosition;
    std::vector<Move> moves;

    EngineMessagePosition(const Position &currentPosition, const Position &startPosition,
                          const std::vector<Move> &moves):EngineMessage(TYPE_POSITION),
        currentPosition(currentPosition), startPosition(
            startPosition), moves(moves) {
    }

    EngineMessagePosition(const Position &currentPosition):EngineMessage(TYPE_POSITION),
        currentPosition(currentPosition), startPosition(), moves() {
    }

    virtual ~EngineMessagePosition() {
    }
};

class CHESSCORE_EXPORT EngineMessageId : public EngineMessage {
public:
    std::string name;
    std::string value;

    EngineMessageId(const std::string &name,
                    const std::string &value):EngineMessage(TYPE_ID), name(name), value(value) {
    }

    virtual ~EngineMessageId() {
    }
};

class CHESSCORE_EXPORT EngineMessageBestMove : public EngineMessage {
public:
    Move bestMove;
    Move ponderMove;
    unsigned thinkingTime;

    EngineMessageBestMove(Move bestMove, Move ponderMove):EngineMessage(TYPE_BEST_MOVE), bestMove(bestMove), ponderMove(
            ponderMove) {
    }

    virtual ~EngineMessageBestMove() {
    }
};

class CHESSCORE_EXPORT EngineMessageInfoSearch : public EngineMessage {
public:
    enum HaveBitmask {
        HAVE_NONE       = 0x0000,
        HAVE_SCORE      = 0x0001,
        HAVE_MATESCORE  = 0x0002,
        HAVE_DEPTH      = 0x0004,
        HAVE_SELDEPTH   = 0x0008,
        HAVE_TIME       = 0x0010,
        HAVE_NODES      = 0x0020,
        HAVE_NPS        = 0x0040,
        HAVE_PV         = 0x0080    // Means pv *and* pvStr are set
    };

    unsigned have;
    int score;
    int mateScore;
    int depth;
    int selectiveDepth;
    int time;
    int64_t nodes;
    int64_t nps;
    std::vector<Move> pv;
    std::string pvStr;                  // String representation of 'pv'

    EngineMessageInfoSearch():EngineMessage(TYPE_INFO_SEARCH), have(HAVE_NONE), score(0), mateScore(0), depth(0),
        selectiveDepth(0), time(0), nodes(0), nps(0), pv(), pvStr() {
    }

    virtual ~EngineMessageInfoSearch() {
    }

    std::string format() const;
};

class CHESSCORE_EXPORT EngineMessageInfoString : public EngineMessage {
public:
    std::string info;

    EngineMessageInfoString(const std::string &info):EngineMessage(TYPE_INFO_STRING), info(info) {
    }

    virtual ~EngineMessageInfoString() {
    }
};

class CHESSCORE_EXPORT EngineMessageOption : public EngineMessage {
public:
    UCIEngineOption option;

    EngineMessageOption(const UCIEngineOption &option):EngineMessage(TYPE_OPTION), option(option) {
    }

    virtual ~EngineMessageOption() {
    }
};

class CHESSCORE_EXPORT EngineMessageCustom : public EngineMessage {
public:
    std::string uci;

    EngineMessageCustom(const std::string &uci):EngineMessage(TYPE_CUSTOM), uci(uci) {
    }

    virtual ~EngineMessageCustom() {
    }
};

class CHESSCORE_EXPORT EngineMessageError : public EngineMessage {
public:
    std::string error;

    EngineMessageError(const std::string &error):EngineMessage(TYPE_ERROR), error(error) {
    }

    virtual ~EngineMessageError() {
    }
};

#define NEW_ENGINE_MESSAGE(type) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessage(ChessCore::EngineMessage::type))

#define NEW_ENGINE_MESSAGE_DEBUG(debug) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageDebug(debug))

#define NEW_ENGINE_MESSAGE_REGISTER(name, code) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageRegister(name, code))

#define NEW_ENGINE_MESSAGE_SET_OPTION(name, value) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageSetOption(name, value))

#define NEW_ENGINE_MESSAGE_POSITION(curr, start, moves) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessagePosition(curr, start, moves))

#define NEW_ENGINE_MESSAGE_ID(name, value) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageId(name, value))

#define NEW_ENGINE_MESSAGE_BEST_MOVE(bestMove, ponderMove) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageBestMove(bestMove, ponderMove))

#define NEW_ENGINE_MESSAGE_INFO_SEARCH(info) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageInfoSearch())

#define NEW_ENGINE_MESSAGE_INFO_STRING(info) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageInfoString(info))

#define NEW_ENGINE_MESSAGE_OPTION(option) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageOption(option))

#define NEW_ENGINE_MESSAGE_CUSTOM(uci) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageCustom(uci))

#define NEW_ENGINE_MESSAGE_ERROR(message) \
    std::shared_ptr<ChessCore::EngineMessage> (new ChessCore::EngineMessageError(message))

}   // namespace ChessCore
