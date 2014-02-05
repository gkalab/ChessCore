//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Log.h: Log functions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Mutex.h>
#include <sstream>

#ifdef USE_ASL_LOGGING
#include <asl.h>
#endif // USE_ASL_LOGGING

namespace ChessCore {

class Blob;

class CHESSCORE_EXPORT Log {
private:
    static const char *m_classname;

public:
    enum Level {
        LEVEL_VERBOSE,
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        //
        MAX_LEVEL
    };

    enum Language {
        LANG_CPP,
        LANG_OBJC
    };

protected:
    static Mutex m_mutex;
    static bool m_debugAllowed;

#ifdef USE_ASL_LOGGING
    static aslclient m_aslClient;
#else // !USE_ASL_LOGGING
    static std::string m_filename;
    static FILE *m_fp;
    static bool m_messingWithLog;
    static unsigned m_numOldFiles;
    static const char *m_levelsText[LEVEL_ERROR + 1];
#endif // USE_ASL_LOGGING

public:
#ifdef USE_ASL_LOGGING
    /**
     * Open a connection to the ASL logging facility.
     *
     * @param facility The facility used during logging.
     *
     * @return true if the connection was opened successfully, else false.
     */
    static bool open(const std::string &facility);

#else // !USE_ASL_LOGGING
    /**
     * Open the specified log file.
     *
     * @param filename The name of the log file to open.  If this is "stderr" then log
     * entries are written to stderr.
     * @param append If true, then append an already existing file, else if false, truncate
     * an existing file or create a new file.
     *
     * @return true if the log file opened successfully, else false.
     */
    static bool open(const std::string &filename, bool append);
#endif // USE_ASL_LOGGING

    /**
     * Determine if the log file is open.
     *
     * @return true if the log file is open, else false.
     */
    static inline bool isOpen() {
#ifdef USE_ASL_LOGGING
        return m_aslClient != NULL;
#else // !USE_ASL_LOGGING
        return m_fp != NULL;
#endif // USE_ASL_LOGGING
    }

#ifndef USE_ASL_LOGGING
    static const std::string &filename() {
        return m_filename;
    }
#endif // !USE_ASL_LOGGING

    /**
     * Close the ASL connection/log file.
     */
    static void close();

    /**
     * @return true if debug logging is allowed, else false.
     */
    static inline bool allowDebug() {
        return m_debugAllowed;
    }

    /**
     * Enabled/disable LOG_DEBUG messages.
     *
     * @param allow true to allow LOG_DEBUG messages to be logged, else false
     * to disallow LOG_DEBUG messages.
     */
    static void setAllowDebug(bool allow);

#ifndef USE_ASL_LOGGING
    /**
     * @return The number of 'old files' that are saved when a new log file is opened.
     */
    static inline unsigned numOldFiles() {
        return m_numOldFiles;
    }

    /**
     * Set the number of 'old files' that are saved when a new log file is opened.
     * (default is 3).
     *
     * @param numOldFiles The number of old files saved.
     */
    static inline void setNumOldFiles(unsigned numOldFiles) {
        m_numOldFiles = numOldFiles;
    }
#endif // !USE_ASL_LOGGING

    /**
     * Log a message to the logfile.  If the logfile is closed or if the message
     * is being logged as LOG_DEBUG level and debug messages are disabled, then
     * the message is discarded.  This method is designed for logging from within
     * a C++ class method or function.
     *
     * @param classname The name of the class logging the message (may be NULL or empty).
     * @param methodname The name of the method logging the message (may be NULL or empty).
     * @param level The logging level.
     * @param message A printf-style formatting string, used to format any optional
     * elements.
     */
    static void log(const char *classname, const char *methodname, Level level, Language language,
        const char *message, ...);

    /**
     * Log a message to the logfile.  If the logfile is closed or if the message
     * is being logged as LOG_DEBUG level and debug messages are disabled, then
     * the message is discarded.  This method is designed for logging from within
     * a C++ class method or function.
     *
     * @param classname The name of the class logging the message (may be NULL or empty).
     * @param methodname The name of the method logging the message (may be NULL or empty).
     * @param level The logging level.
     * @param message The message to log.
     */
    static void log0(const char *classname, const char *methodname, Level level, Language language,
        const char *message);

    /**
     * Log a message without classname, methods, level or language.
     *
     * @param message The message to log.
     */
    static void logbare(const char *message);

#ifndef USE_ASL_LOGGING
    /**
     * Get a "snapshot" of the current log file contents.
     *
     * @param contents Where to store the log file contents.
     *
     * @return true if the log file contents was retrieved successfully, else false.
     */
    static bool snapshot(Blob &contents);
#endif // USE_ASL_LOGGING

    /**
     * Log a stack trace.
     *
     * @param message Message to write before stack trace.
     */
    static void logStacktrace(const char *message = 0);
};

//
// Perform logging via stream interface
//
class CHESSCORE_EXPORT StreamLog {
protected:
    std::ostringstream m_stream;
    const char *m_classname;
    const char *m_methodname;
    Log::Level m_level;

public:
    StreamLog();
    virtual ~StreamLog();

    std::ostringstream &get(const char *classname, const char *methodname, Log::Level level);

    // No copy
private:
    StreamLog(const StreamLog &);
    StreamLog &operator=(const StreamLog &);
};

// Objective-C has its own versions of these macros, defined elsewhere...
#ifndef __OBJC__
// Define VERBOSE_LOGGING to true before including this header
#ifndef VERBOSE_LOGGING
#define VERBOSE_LOGGING 0
#endif

#if VERBOSE_LOGGING
#define logverbose(fmt, ...) ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_VERBOSE, \
                                                 ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#else  // !VERBOSE_LOGGING
#define logverbose(fmt, ...) /* nothing */
#endif // VERBOSE_LOGGING

#define logdbg(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_DEBUG, \
                                                 ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define loginf(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_INFO, \
                                                 ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define logwrn(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_WARNING, \
                                                 ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define logerr(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_ERROR, \
                                                 ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)

#define LOGVERBOSE \
    if (!VERBOSE_LOGGING || !ChessCore::Log::allowDebug()) ; \
    else ChessCore::StreamLog().get(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_VERBOSE)

#define LOGDBG \
    if (!ChessCore::Log::allowDebug()) ; \
    else ChessCore::StreamLog().get(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_DEBUG)

#define LOGINF ChessCore::StreamLog().get(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_INFO)
#define LOGWRN ChessCore::StreamLog().get(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_WARNING)
#define LOGERR ChessCore::StreamLog().get(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_ERROR)

#ifdef TESTING
#define tlogdbg(fmt, ...) ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_DEBUG, \
ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define tloginf(fmt, ...) ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_INFO, \
ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define tlogwrn(fmt, ...) ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_WARNING, \
ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define tlogerr(fmt, ...) ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_ERROR, \
ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)

#else // !TESTING
#define tlogdbg(fmt, ...) /* nothing */
#define tloginf(fmt, ...) /* nothing */
#define tlogwrn(fmt, ...) /* nothing */
#define tlogerr(fmt, ...) /* nothing */
#endif // TESTING

#endif // !__OBJC__
} // namespace ChessCore
