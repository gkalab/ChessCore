//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ccore.cpp: 'ccore' entry point.
//

#include "ccore.h"
#include <ChessCore/ProgOption.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Game.h>
#include <ChessCore/Log.h>
#include <ChessCore/Database.h>
#include <ChessCore/Version.h>

#include <stdlib.h>
#include <signal.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>

#ifndef WINDOWS
#include <execinfo.h>
#endif // !WINDOWS

using namespace std;
using namespace ChessCore;

#define DEBUG_COMMAND_LINE 0

static const char *m_classname = 0;

string g_progName;
string g_optCfgFile;
int g_optDepth = 0;
bool g_optDebugLog = false;
string g_optDotDir;
string g_optEpdFile;
string g_optEcoFile;
string g_optFen;
bool g_optHelp = false;
string g_optInputDb;
uint64_t g_optKey = 0ULL;
string g_optLogFile;
bool g_optLogComms = false;
int g_optNumber1 = 0;
bool g_optNumber1Ind = false;
int g_optNumber2 = 0;
bool g_optNumber2Ind = false;
string g_optOutputDb;
bool g_optQuiet = false;
bool g_optRelaxed = false;
TimeControl g_optTimeControl;
string g_optTimeStr;
bool g_optVersion = false;

static ProgOption g_options[] = {
    ProgOption('c', "cfgfile",      false,  &g_optCfgFile),
    ProgOption('d', "depth",        false,  &g_optDepth),
    ProgOption('D', "debuglog",     false,  &g_optDebugLog),
    ProgOption(0,   "dotdir",       false,  &g_optDotDir),
    ProgOption('e', "epdfile",      false,  &g_optEpdFile),
    ProgOption('E', "ecofile",      false,  &g_optEcoFile),
    ProgOption('f', "fen",          false,  &g_optFen),
    ProgOption('h', "help",         false,  &g_optHelp),
    ProgOption('i', "indb",         false,  &g_optInputDb),
    ProgOption('k', "key",          false,  &g_optKey),
    ProgOption('l', "logfile",      false,  &g_optLogFile),
    ProgOption('L', "logcomms",     false,  &g_optLogComms),
    ProgOption('n', "number1",      false,  &g_optNumber1, &g_optNumber1Ind),
    ProgOption('N', "number2",      false,  &g_optNumber2, &g_optNumber2Ind),
    ProgOption('o', "outdb",        false,  &g_optOutputDb),
    ProgOption('r', "relaxed",      false,  &g_optRelaxed),
    ProgOption('q', "quiet",        false,  &g_optQuiet),
    ProgOption('t', "timecontrol",  false,  &g_optTimeStr),
    ProgOption('v', "version",      false,  &g_optVersion),
    ProgOption()
};

bool g_quitFlag = false;
IoEvent g_quitEvent;

static bool run(const vector<string> &args);
static void usage(ostream &stream);
static void writeProgramInfo(ostream &stream);
static void writeVersion(ostream &stream);

#ifdef WINDOWS
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType);
#else
static void signalHandler(int sig);
#endif

int main(int argc, const char **argv) {
    const string method = "main";
    bool success = false;

#if DEBUG_COMMAND_LINE
    cout << "argc=" << argc << endl;
    cout << "argv=";

    for (int i = 0; i < argc; i++)
        cout << argv[i] << " ";
    cout << endl;
#endif // DEBUG_COMMAND_LINE

    try {

#ifdef WINDOWS
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else // !WINDOWS
        // Set-up signal handler
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
        sigfillset(&sa.sa_mask);
        sigaction(SIGINT, &sa, 0);
        sigaction(SIGTERM, &sa, 0);
        sigaction(SIGHUP, &sa, 0);
        sigaction(SIGSEGV, &sa, 0);
        sigaction(SIGBUS, &sa, 0);
        sigaction(SIGILL, &sa, 0);
        sigaction(SIGFPE, &sa, 0);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGPIPE, SIG_IGN);
#endif // WINDOWS

        vector<string> trailingArgs;
        string errorMsg;

        if (!ProgOption::parse(g_options, argc, argv, g_progName, trailingArgs, errorMsg)) {
            writeProgramInfo(cerr);
            cerr << errorMsg << endl;
            return 1;
        } else if (g_optHelp) {
            writeProgramInfo(cout);
            usage(cout);
            return 0;
        } else if (g_optVersion) {
            writeVersion(cout);
            return 0;
        }

        if (!g_optCfgFile.empty())
            if (!Config::read(g_optCfgFile)) {
                cerr << "Error reading config file '" << g_optCfgFile << "'" << endl;
                return 2;
            }

        if (!g_optDotDir.empty() && !Util::dirExists(g_optDotDir)) {
            cerr << "Directory '" << g_optDotDir << " does not exist" << endl;
            return 3;
        }

        if (g_optLogComms && !g_optDebugLog)
            g_optDebugLog = true;

        if (!g_optTimeStr.empty()) {
            if (!g_optTimeControl.set(g_optTimeStr)) {
                cerr << "Invalid time control '" << g_optTimeStr << "' specified" << endl;
                return 4;
            }
        }

        if (!g_optQuiet)
            writeProgramInfo(cout);

        // Initialise ChessCore *after* command line options have been parsed but
        // before the logfile has been opened and debug mode set
        if (!init())
            return 6;

        Game::setRelaxedMode(g_optRelaxed);

        // Always use a log file
        if (g_optLogFile.empty())
            g_optLogFile = g_tempDir + PATHSEP + "ccore.log";

        Log::open(g_optLogFile, false);
        Log::setAllowDebug(g_optDebugLog);

        if (!g_optQuiet && Log::isOpen()) {
            cout << "Using log file '" << Log::filename() << "'" << endl;
        }

        LOGDBG << g_platform << " " << g_buildType << " " << g_cpu <<
            ". Compiled " << g_buildTime << " using " << g_compiler;
        LOGDBG << (usingCpuPopcnt() ? "Using" : "Not Using") << " CPU POPCNT instruction";

        success = run(trailingArgs);

    } catch(ChessCoreException &e) {
        cerr << "ChessCore exception: " << e.what() << endl;
    } catch(std::exception &e) {
        cerr << "Runtime exception: " << e.what() << endl;
    } catch(...) {
        cerr << "Unknown exception" << endl;
    }

    // Delete configurations
    Config::clear();

    fini();

    return success ? 0 : 10;
}

//
// ♪♪♪ You better make your face up in your favourite disguise ♪♪♪
//
static bool run(const vector<string> &args) {
#if DEBUG_COMMAND_LINE
    cout << "args.size=" << args.size() << endl;
    cout << "args=";

    for (auto it = args.begin(); it != args.end(); ++it)
        cout << *it << " ";
    cout << endl;
#endif // DEBUG_COMMAND_LINE

    if (args.size() == 1) {
        if (args[0] == "random")
            return funcRandom(false);
        else if (args[0] == "crandom")
            return funcRandom(true);
        else if (args[0] == "randompos")
            return funcRandomPositions();
        else if (args[0] == "makeepd")
            return funcMakeEpd();
        else if (args[0] == "validatedb")
            return funcValidateDb();
        else if (args[0] == "copydb")
            return funcCopyDb();
        else if (args[0] == "buildoptree")
            return funcBuildOpeningTree();
        else if (args[0] == "classify")
            return funcClassify();
        else if (args[0] == "pgnindex")
            return funcPgnIndex();
        else if (args[0] == "searchdb")
            return funcSearchDb();
        else if (args[0] == "perftdiv")
            return funcPerftdiv();
        else if (args[0] == "recursiveposdump")
            return funcRecursivePosDump();
        else if (args[0] == "findbuggypos")
            return funcFindBuggyPos();
        else if (args[0] == "testpopcnt")
            return funcTestPopCnt();
    } else if (args.size() == 2) {
        if (args[0] == "analyze")
            return analyzeGames(args[1]);
        else if (args[0] == "processepd")
            return processEpd(args[1]);
    } else if (args.size() == 3) {
        if (args[0] == "tournament" ||
            args[0] == "playgames")
            return playGames(args[1], args[2]);
    }

    LOGERR << "No command specified!";

    usage(cerr);
    return false;
}

static void usage(ostream &stream) {
    stream << "usage: ccore [options] FUNCTION [ENGINE [ENGINE]]\n";
    stream << "\n";
    stream << "options:\n";
    stream << "-c, --cfgfile=FILE         Engine configuration file [ccore.cfg].\n";
    stream << "-d, --depth=NUM            Depth variable.\n";
    stream << "-D, --debuglog             Turn on debug logging.\n";
    stream << "    --dotdir=DIR           Dump final game tree to .dot files in directory.\n";
    stream << "-e, --epdfile=FILE         EPD file.\n";
    stream << "-E, --ecofile=FILE         ECO Classification file.\n";
    stream << "-f, --fen=FEN              Position in Forsyth-Edwards Notation.\n";
    stream << "-h, --help                 Print this help text.\n";
    stream << "-i, --indb=FILE            Input database.\n";
    stream << "-k, --key=KEY              64-bit key variable.\n";
    stream << "-l, --logfile=FILE         Log file.\n";
    stream << "-L, --logcomms             Log engine communication (turns on debug logging).\n";
    stream << "-n, --number1=NUM          Integer variable #1.\n";
    stream << "-N, --number2=NUM          Integer variable #2.\n";
    stream << "-o, --outdb=FILE           Output database\n";
    stream << "-q, --quiet                Don't print program info during start-up.\n";
    stream << "-r, --relaxed              Allow errors.\n";
    stream << "-t, --timecontrol=TIME     Time control, for example \"40/120;G/20\" or \"300+10:1800\".\n";
    stream << "-v, --version              Write program version in machine-readable format.\n";
    stream << "\n";
    stream << "FUNCTION: tournament ENGINE ENGINE. -c, -t, -n=num games, [-o]\n";
    stream << "          analyze ENGINE. -c, -i, -o, -d/-t, [-n=first game, -N=last game].\n";
    stream << "          processepd ENGINE. -c, -e, [-n=first epd, -N=last epd].\n";
    stream << "          random: Generate random numbers. -n=count.\n";
    stream << "          crandom: Generate random numbers in C-format. -n=count.\n";
    stream << "          randompos: Generate random positions. [-n]\n";
    stream << "          makeepd: Generate EPD from a database. -e, -i.\n";
    stream << "          validatedb: Validate a database. -i, [-n=first game, -N=last game].\n";
    stream << "          copydb: Copy a database. -i, -o, [-n=first game, -N=last game].\n";
    stream << "          buildoptree: Build Opening Tree. -i, [-n=first game, -N=last game, -d].\n";
    stream << "          classify: Classify openings. -i, -E, [-n=first game, -N=last game].\n";
    stream << "          pgnindex: Get PGN index info. -i, [-n=first game, -N=last game].\n";
    stream << "          searchdb: Search database. -i.\n";
    stream << "          perftdiv: Print perft by top-level mode. -f, -d\n";
    stream << "          recursiveposdump: Recursive dump the positions FENs. -f, -d\n";
    stream << "          findbuggypos: Interactive mode used with tools/find_buggy_pos.py\n";
    stream << "          testpopcnt: Test popcnt performance. -n=iterations.\n";
}

static void writeProgramInfo(ostream &stream) {
    stream << "ChessCore Test Tool (ccore). Copyright (c)2008-2013 Andy Duplain <andy@trojanfoe.com>" <<
        endl;
    stream << "ChessCore v" << g_version << " (" << g_build << ") " << g_cpu << " " << g_buildType <<
        endl;
    stream << endl;
}

static void writeVersion(ostream &stream) {
    stream << g_version << "_" << g_build << endl;
}

#ifdef WINDOWS

static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    static unsigned count = 0;
    if (count++ == 0 && !g_quitFlag) {
        LOGINF << "Quitting on console event " << hex << ctrlType;
        setQuit();
    } else {
        if (count < 3) {
            LOGWRN << "Ignoring console event " << hex << ctrlType;
        } else {
            LOGERR << "Terminating due to impatient user";
            _exit(101);
        }
    }
    return TRUE;
}

#else // !WINDOWS

static void signalHandler(int sig) {
    if (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE) {
        // This is a bad...
        LOGERR << "Terminating on signal " << sig;
        const size_t maxFrames = 128;
        void *frames[maxFrames];
        unsigned numFrames = backtrace(frames, maxFrames);
        Log::dumpStack(frames, numFrames);
        _exit(101);
    }

    static unsigned count = 0;
    if (count++ == 0 && !g_quitFlag) {
        LOGINF << "Quitting on signal " << sig;
        setQuit();
    } else {
        if (count < 3) {
            LOGWRN << "Ignoring signal " << sig;
        } else {
            LOGERR << "Terminating due to impatient user";
            _exit(101);
        }
    }
}

#endif // WINDOWS

void setQuit() {
    g_quitFlag = true;
    g_quitEvent.set();
}
