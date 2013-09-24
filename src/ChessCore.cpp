//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ChessCore.cpp: Library initialisation and clean-up.
//

#include <ChessCore/ChessCore.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Rand64.h>
#include <ChessCore/Data.h>
#include <ChessCore/Log.h>
#include <ChessCore/Util.h>

#ifdef MACOSX
#include <ChessCore/AppleUtil.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif // MACOSX

#if defined(WINDOWS) && defined(_DEBUG)
#include <crtdbg.h>
#endif // WINDOWS && _DEBUG

using namespace std;

namespace ChessCore {

//static const char *m_classname = "";

static bool beingDebugged();

const string g_platform =
#if defined(WINDOWS)
	"Windows";
#elif defined(MACOSX)
    "MacOSX";
#elif defined(IOS)
    "iOS";
#elif defined(LINUX)
    "Linux";
#elif defined(FREEBSD)
	"FreeBSD";
#else
    "Unknown";
#endif

const string g_buildType =
#if defined(_PROFILE)
    "Profile";
#elif defined(_DEBUG)
    "Debug";
#else
    "Release";
#endif

const string g_cpu =
#if defined(CPU_X64)
    "x64";
#elif defined(CPU_X86)
    "x86";
#elif defined(CPU_ARM)
    "ARM";
#else
    "Unknown";
#endif

const string g_bitSize =
#if BITSIZE == 64
    "64-bit";
#elif BITSIZE == 32
    "32-bit";
#else
    "Unknown";
#endif

#ifdef _MSC_FULL_VER
string msvcVersion() {
    unsigned major =  _MSC_FULL_VER / 10000000;
    unsigned minor = (_MSC_FULL_VER % 10000000) / 100000;
    unsigned patch = (_MSC_FULL_VER % 10000000) % 100000;
    return Util::format("Visual C++ %02u.%02u.%05u", major, minor, patch);
}
#endif // !_MSC_FULL_VER

const string g_compiler =
#if defined(__clang__)
    "Clang " __VERSION__;
#elif defined(__INTEL_COMPILER)
    "Intel C++ " __INTEL_COMPILER;
#elif defined(_MSC_FULL_VER)
    msvcVersion();
#else
    "GNU C++ " __VERSION__;
#endif

const string g_buildTime = __DATE__ " " __TIME__;

string g_tempDir;
string g_docDir;
bool g_beingDebugged = false;

static bool g_initted = false;

bool init() {

    if (g_initted)
        return true;

#if defined(WINDOWS) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_DELAY_FREE_MEM_DF);
#endif // WINDOWS && _DEBUG

    char temp[1024];
    const char *tempdir = 0;

    g_beingDebugged = beingDebugged();

#if defined(WINDOWS)
    if (GetTempPath(sizeof(temp), temp) > 0)
        tempdir = temp;
#elif defined(MACOSX)
    if (appleTempDir(temp, sizeof(temp)))
        tempdir = temp;
#else // !WINDOWS && !MACOSX
    tempdir = getenv("TEMP");
    if (tempdir == 0 || *tempdir == '\0')
        tempdir = "/tmp";
#endif // WINDOWS

    if (tempdir != 0)
        g_tempDir = tempdir;

    // Strip trailing slash
    if (!g_tempDir.empty()) {
        if (g_tempDir[g_tempDir.size() - 1] == PATHSEP)
            g_tempDir = g_tempDir.substr(0, g_tempDir.size() - 1);
    }

    g_docDir = g_tempDir;

    lowlevelInit();

    Rand64::init();

    if (!dataInit())
        return false;

    g_initted = true;
    return true;
}

void fini() {
    if (g_initted) {
#if defined(WINDOWS) && defined(_DEBUG)
        _CrtCheckMemory();

        // This always shows some leaks.  Not sure why yet...
        //if (_CrtDumpMemoryLeaks() > 0) {
        //    LOGERR << "Memory leaks detected!";
        //}
#endif // WINDOWS && _DEBUG
        Log::close();
        g_initted = false;
    }
}


// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
static bool beingDebugged() {
#if defined(WINDOWS)

    return IsDebuggerPresent() == TRUE;

#elif defined(MACOSX)

    int mib[4] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROC_PID,
        (int)getpid()
    };

    struct kinfo_proc info;
    size_t size = sizeof(info);

    memset(&info, 0, sizeof(info));
    int rc = sysctl(mib, sizeof(mib) / sizeof(mib[0]), &info, &size, NULL, 0);

    if (rc < 0)
        return false;

    // We're being debugged if the P_TRACED flag is set.
    return (info.kp_proc.p_flag & P_TRACED) != 0;

#else

    return false;

#endif // MACOSX
}


}   // namespace ChessCore
