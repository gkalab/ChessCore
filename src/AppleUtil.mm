//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// AppleUtil.m: Apple-specific utility functions.
//

#include <Foundation/Foundation.h>
#include <ChessCore/ChessCore.h>
#include <ChessCore/AppleUtil.h>
#include <ChessCore/Log.h>
#include <string.h>

static const char *m_classname = "AppleUtil";

// Log.h is written to assume that any Objective-C code including it will use CocoaUtil.h from ChessFront
// to provide logerr() etc., and so excludes the following macros:
#define logdbg(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_DEBUG, \
                                                ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define loginf(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_INFO, \
                                                ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define logwrn(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_WARNING, \
                                                ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)
#define logerr(fmt, ...)     ChessCore::Log::log(m_classname, __FUNCTION__, ChessCore::Log::LEVEL_ERROR, \
                                                ChessCore::Log::LANG_CPP, fmt, ## __VA_ARGS__)

int CHESSCORE_EXPORT appleTempDir(char *buffer, unsigned buflen) {
    NSString *tempDir = NSTemporaryDirectory();
    if (tempDir) {
        strncpy(buffer, [tempDir UTF8String],  buflen);
        return TRUE;
    }

    logerr("Failed to get temporary directory");
    return FALSE;
}
