//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// EngineMessage.cpp: Engine message class implementation.
//

#include <ChessCore/EngineMessage.h>
#include <sstream>

using namespace std;

namespace ChessCore {
//
// EngineMessage
//
const char *EngineMessage::typeDescs[EngineMessage::NUM_TYPES] = {
    "None",             // TYPE_NONE

    // GUI -> Engine
    "Uci",              // TYPE_UCI
    "Debug",            // TYPE_DEBUG
    "IsReady",          // TYPE_IS_READY
    "Register",         // TYPE_REGISTER
    "SetOption",        // TYPE_SET_OPTION
    "NewGame",          // TYPE_NEW_GAME
    "Position",         // TYPE_POSITION
    "Go",               // TYPE_GO
    "Stop",             // TYPE_STOP
    "PonderHit",        // TYPE_PONDER_HIT
    "Quit",             // TYPE_QUIT

    // Engine -> GUI
    "Id",               // TYPE_ID
    "UciOK",            // TYPE_UCI_OK
    "RegistrationError", // TYPE_REGISTRATION_ERROR
    "ReadyOK",          // TYPE_READY_OK
    "BestMove",         // TYPE_BEST_MOVE
    "InfoSearch",       // TYPE_INFO_SEARCH
    "InfoString",       // TYPE_INFO_STRING
    "Option",           // TYPE_OPTION

    // Special Messages
    "MainloopAlive",    // TYPE_MAINLOOP_ALIVE
    "Custom",           // TYPE_CUSTOM
    "Error"             // TYPE_ERROR
};

const char *EngineMessage::typeDesc(EngineMessage::Type type) {
    if (type < NUM_TYPES)
        return typeDescs[type];

    return "Unknown";
}

//
// EngineMessageInfoSearch
//
string EngineMessageInfoSearch::format() const {
    ostringstream oss;

    if (have & HAVE_DEPTH) {
        oss << dec << depth;

        if (have & HAVE_SELDEPTH)
            oss << "/" << dec << selectiveDepth;

        oss << ' ';
    }

    if (have & HAVE_TIME)
        oss << Util::formatElapsed(time) << "s ";

    if (have & HAVE_MATESCORE)
        oss << '#' << dec << mateScore << ' ';
    else if (have & HAVE_SCORE)
        oss << Util::formatCenti(score) << ' ';

    if ((have & (HAVE_NODES | HAVE_NPS)) == (HAVE_NODES | HAVE_NPS))
        oss << dec << nodes << " (" << nps / 1000LL << "knps) ";

    if (have & HAVE_PV)
        oss << pvStr;

    return oss.str();
}
}   // namespace ChessCore
