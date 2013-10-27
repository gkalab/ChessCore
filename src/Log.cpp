//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Log.cpp: Log class implementation.
//

#include <ChessCore/Log.h>
#include <ChessCore/Util.h>
#include <ChessCore/Thread.h>
#include <ChessCore/Blob.h>
#include <stdio.h>
#include <string.h>

#ifndef WINDOWS
#include <execinfo.h>
#include <errno.h>
#endif // !WINDOWS

using namespace std;

namespace ChessCore {

// ==========================================================================
// class Log
// ==========================================================================

const char *Log::m_classname = "Log";
string Log::m_filename = "stderr";
Mutex Log::m_mutex;
FILE *Log::m_fp = stderr;
bool Log::m_debugAllowed = DEBUG_TRUE;
bool Log::m_messingWithLog = false;
unsigned Log::m_numOldFiles = 3;
const char *Log::m_levelsText[MAX_LEVEL] = {
    "VRB ",
    "DBG ",
    "INF ",
    "WRN ",
    "ERR "
};

bool Log::open(const string &filename, bool append) {
    close();
    m_filename = filename;

    if (m_filename == "stderr") {
        m_fp = stderr;
    } else {
        if (m_numOldFiles > 0 &&
            (!append || !Util::fileExists(filename))) {
            // We will be creating a new log file
            string directory = Util::dirName(filename);
            string basename = Util::baseName(filename);
            size_t pos = basename.find_last_of('.');
            string extension = basename.substr(pos);
            basename = basename.substr(0, pos);

            for (unsigned history = m_numOldFiles; history > 0; history--) {
                string oldFilename = (history == 1) ? filename :
                                     Util::format("%s/%s-%u%s", directory.c_str(),
                                                  basename.c_str(), history - 1, extension.c_str());
                string newFilename = Util::format("%s/%s-%u%s", directory.c_str(), basename.c_str(), history,
                                                  extension.c_str());
                Util::deleteFile(newFilename);
                Util::renameFile(oldFilename, newFilename);
            }
        }

        m_fp = fopen(m_filename.c_str(), append ? "a+" : "w+");

        if (m_fp)
            loginf(">>>> opened");
    }

    return m_fp != NULL;
}

void Log::close() {
    if (m_fp  && m_fp != stderr) {
        loginf("<<<< closed");
        fclose(m_fp);
    }

    m_filename.clear();
    m_fp = NULL;
}

void Log::log(const char *classname, const char *methodname, Level level, Language language, const char *message, ...) {
    if (!isOpen() || (level == LEVEL_DEBUG && !m_debugAllowed))
        return;

    char buffer[8192];
    va_list va;
    va_start(va, message);
    vsprintf(buffer, message, va);
    va_end(va);

    return log0(classname, methodname, level, language, buffer);
}

inline char *append(char *buffer, const char *str) {
    size_t len = strlen(str);

    strncpy(buffer, str, len);
    return buffer + len;
}

void Log::log0(const char *classname, const char *methodname, Level level, Language language, const char *message) {
    if (m_messingWithLog)
        return;     // Doing something with the log file data (probably Log::snapshot()).
    MutexLock lock(m_mutex);

    if (!isOpen() || (level == LEVEL_DEBUG && !m_debugAllowed))
        return;

    char prefix[256], *p = prefix, *afterDateTime = 0;

    if (m_fp != stderr) {
        string timestamp = Util::formatTime(true);
        p = append(p, timestamp.c_str());
        p = append(p, " ");
    }

    afterDateTime = p;

    string threadStr =
#if BITSIZE == 32 || defined(WINDOWS)
        Util::format("(%08lx) ", Thread::currentThreadId());
#elif BITSIZE == 64
        Util::format("(%016llx) ", Thread::currentThreadId());
#endif
    p = append(p, threadStr.c_str());

    p = append(p, m_levelsText[level]);

#ifndef WINDOWS    // Under Visual C++ __FUNCTION__ expands to namespace::class::function
    if (classname && *classname)
        p = append(p, classname);
#endif // !WINDOWS

    if (methodname && *methodname) {
#ifndef WINDOWS
        if (language == LANG_CPP)
            p = append(p, "::");
        else if (language == LANG_OBJC) {
            if (classname && *classname)
                p = append(p, "  ");
        } else {
            p = append(p, ".");
        }
#endif // !WINDOWS
        p = append(p, methodname);
    }

    if (language == LANG_OBJC)
        p = append(p, " ");
    else
        p = append(p, ": ");

    *p = '\0';
    fwrite(prefix, 1, p - prefix, m_fp);

    if (g_beingDebugged && m_fp != stderr)
        fwrite(afterDateTime, 1, p - afterDateTime, stderr);

    if (message && *message) {
        size_t len = strlen(message);
        fwrite(message, 1, len, m_fp);

        if (g_beingDebugged && m_fp != stderr)
            fwrite(message, 1, len, stderr);
    }

    fwrite("\n", 1, 1, m_fp);

    if (g_beingDebugged && m_fp != stderr)
        fwrite("\n", 1, 1, stderr);

    fflush(m_fp);

    if (g_beingDebugged && m_fp != stderr)
        fflush(stderr);
}

void Log::logbare(const char *message) {
    if (m_messingWithLog)
        return;     // Doing something with the log file data (probably Log::snapshot()).
    MutexLock lock(m_mutex);

    if (message == 0 || !isOpen())
        return;

    size_t len = strlen(message);

    if (len > 0) {
        fwrite(message, 1, len, m_fp);

        if (g_beingDebugged && m_fp != stderr)
            fwrite(message, 1, len, stderr);

        if (message[len - 1] != '\n') {
            fwrite("\n", 1, 1, m_fp);

            if (g_beingDebugged && m_fp != stderr)
                fwrite("\n", 1, 1, stderr);
        }

        fflush(m_fp);

        if (g_beingDebugged && m_fp != stderr)
            fflush(stderr);
    }
}

bool Log::snapshot(Blob &contents) {

    if (!isOpen())
        return false;

    // Stop logging by setting the m_messingWithLog
    MutexLockWithBool lock(m_mutex, m_messingWithLog);

    contents.free();

    size_t size = (size_t)ftell(m_fp);
    if (!contents.reserve((unsigned)size))
        return false;
    contents.setLength((unsigned)size);     // Blob::reserve() doesn't set the length

    fseek(m_fp, 0, SEEK_SET);
    size_t numRead = fread(contents.data(), 1, size, m_fp);
    fseek(m_fp, (long)size, SEEK_SET);

    return (numRead == size);
}

void Log::logStacktrace(const char *message /*=0*/) {
#ifdef MACOSX
    const size_t maxFrames = 128;
    void *frames[maxFrames];
    unsigned numFrames = backtrace(frames, maxFrames);
    char **frameStrings = backtrace_symbols(frames, numFrames);

    if (message)
        logbare(message);

    if (frameStrings) {
        for (unsigned i = 1; i < numFrames && frameStrings[i]; i++) {
            logbare(frameStrings[i]);
        }
        free(frameStrings);
    }
#endif // MACOSX
}

// ==========================================================================
// class StreamLog
// ==========================================================================

StreamLog::StreamLog() : 
    m_stream(),
    m_classname(0),
    m_methodname(0),
    m_level(Log::LEVEL_INFO) {
}

StreamLog::~StreamLog() {
    if (m_stream.tellp() > 0)
        Log::log0(m_classname, m_methodname, m_level, Log::LANG_CPP, m_stream.str().c_str());
}

std::ostringstream &StreamLog::get(const char *classname, const char *methodname, Log::Level level) {
    m_classname = classname;
    m_methodname = methodname;
    m_level = level;
    return m_stream;
}

}   // namespace ChessCore
