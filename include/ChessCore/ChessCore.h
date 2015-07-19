//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ChessCore.h: ChessCore core include file.
//

#pragma once

//
// Platform/CPU specifics
//
#if defined(_WIN32)
//
// Windows
//

// warning C4251: class X needs to have dll-interface to be used by clients of class Y
#pragma warning(disable: 4251)

// warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#pragma warning(disable: 4290)

// warning C4996: X: The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _X
#pragma warning(disable: 4996)

#if defined(_M_X64)
#define CPU_X64			1
#define ASMCALL			__fastcall
#define BITSIZE			64
#define LITTLE_ENDIAN	1
#elif defined(_M_IX86)
#define CPU_X86			1
#define ASMCALL			__cdecl
#define BITSIZE			32
#define LITTLE_ENDIAN	1
#else
#error Unsupported Windows platform
#endif // _M_X64

#define WINDOWS 1
#define PATHSEP '\\'

#elif defined(__APPLE__)
//
// OSX and iOS
//

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE

#if defined(__i386)
#define CPU_X86           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#elif defined(__arm)
#define CPU_ARM           1
#define BITSIZE           32
#define ASMCALL           /* nothing */
#else
#error Unsupported iOS platform
#endif

#define IOS               1
#define UNIX              1
#define PATHSEP           '/'
#define IO_EVENT_USE_POLL 1

#else // !TARGET_OS_IPHONE

#if defined(__x86_64)
#define CPU_X64           1
#define BITSIZE           64
#define ASMCALL           /* nothing */
#elif defined(__i386)
#define CPU_X86           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#else
#error Unsupported OSX platform
#endif

#define MACOSX            1
#define UNIX              1
#define PATHSEP           '/'
#define IO_EVENT_USE_POLL 1
#endif // TARGET_OS_IPHONE

#elif defined(__linux)
//
// Linux
//

// Note: LITTLE_ENDIAN etc. is already defined by the system headers under
//       Linux.

#if defined(__x86_64)
#define CPU_X64           1
#define BITSIZE           64
#define ASMCALL           /* nothing */
#elif defined(__i386)
#define CPU_X86           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#elif defined(__arm__)
#define CPU_ARM           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#else
#error Unsupported Linux platform
#endif

#define LINUX             1
#define UNIX              1
#define PATHSEP           '/'
#define IO_EVENT_USE_POLL 1
#define _FILE_OFFSET_BITS 64     // For fseeko() etc.

#elif defined(__FreeBSD__)
//
// FreeBSD
//

#if defined(__x86_64)
#define CPU_X64           1
#define BITSIZE           64
#define ASMCALL           /* nothing */
#elif defined(__i386)
#define CPU_X86           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#elif defined(__arm__)
#define CPU_ARM           1
#define BITSIZE           32
#define ASMCALL           __attribute__((cdecl))
#else
#error Unsupported FreeBSD platform
#endif

#define FREEBSD           1
#define UNIX              1
#define PATHSEP           '/'
#define IO_EVENT_USE_POLL 1

#else
//
// Unsupported platform
//

#error Unsupported platform

#endif

#ifdef __MINGW32__
#undef CPU_X86
#undef CPU_X64
#endif

// Select whether to use assembler, built-in or custom low-level functions
#if CPU_X64

#undef USE_BUILTIN_POPCNT
#define USE_BUILTIN_BSWAP  1

#elif CPU_X86

#define USE_BUILTIN_POPCNT 1
#define USE_BUILTIN_BSWAP  1

#elif CPU_ARM

#define USE_BUILTIN_POPCNT 1
#define USE_BUILTIN_BSWAP  1
#endif

//
// Platform-specific headers
//
#ifdef WINDOWS

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#ifdef __MINGW32__
#define CHESSCORE_EXPORT __attribute__ ((visibility("default")))
#else //!__MINGW32__
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define strdup(s) _strdup(s)
#define strtoll(s, b, r) _strtoi64(s, b, r)
#define strtoull(s, b, r) _strtoui64(s, b, r)
#define fseeko(f, o, w) _fseeki64(f, o, w)
#define ftello(f) _ftelli64(f)

#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT

typedef SSIZE_T ssize_t;

#ifdef CHESSCORE_EXPORTS
#define CHESSCORE_EXPORT __declspec(dllexport)
#else
#define CHESSCORE_EXPORT __declspec(dllimport)
#endif
#endif // __MINGW32__

#else // !WINDOWS

#include <unistd.h>

#define CHESSCORE_EXPORT __attribute__ ((visibility("default")))

#endif // WINDOWS

//
// Universal headers
//

#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <exception>

namespace ChessCore {

extern CHESSCORE_EXPORT const std::string g_platform;
extern CHESSCORE_EXPORT const std::string g_buildType;
extern CHESSCORE_EXPORT const std::string g_cpu;
extern CHESSCORE_EXPORT const std::string g_bitSize;
extern CHESSCORE_EXPORT const std::string g_compiler;
extern CHESSCORE_EXPORT const std::string g_buildTime;
extern CHESSCORE_EXPORT bool g_beingDebugged;

// Where temporary files are created
extern CHESSCORE_EXPORT std::string g_tempDir;

// Where document files are created
extern CHESSCORE_EXPORT std::string g_docDir;

//
// Initialise data structures.
//
extern CHESSCORE_EXPORT bool init();

//
// Clean-up data structures.
//
extern CHESSCORE_EXPORT void fini();

//
// ChessCoreException
//
class CHESSCORE_EXPORT ChessCoreException : public std::exception {
protected:
    std::string m_reason;

public:
    ChessCoreException();
    ChessCoreException(const char *reason, ...);
    ChessCoreException(const std::string &reason);
    virtual ~ChessCoreException() throw();

    const char *what() const throw();
};

#ifndef __OBJC__        // Objective-C uses a different implementation of ASSERT()
#ifdef DEBUG
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            logerr("Assertion '%s' failed at %s:%d", #condition, __FILE__, __LINE__); \
            throw ChessCore::ChessCoreException("Assertion '%s' failed at %s:%d", #condition, __FILE__, __LINE__); \
        } \
    } while (0)

#else // !DEBUG
#define ASSERT(condition) /* nothing */
#endif // DEBUG
#endif // __OBJC__
} // namespace ChessCore

#include "Types.h"
