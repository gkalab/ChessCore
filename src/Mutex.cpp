//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Mutex.cpp: Mutex class implementation.
//

#include <ChessCore/Mutex.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>
#include <string.h>

#ifndef WINDOWS
#include <errno.h>
#endif // !WINDOWS

namespace ChessCore {

#ifndef WINDOWS
static bool attrInitted = false;
static pthread_mutexattr_t attr;

#endif // !WINDOWS

Mutex::Mutex() {
#ifdef WINDOWS

    InitializeCriticalSection(&m_critical_section);

#else

    if (!attrInitted) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        attrInitted = true;
    }

    if (pthread_mutex_init(&m_mutex, &attr) < 0) {
        throw ChessCoreException("Failed to create mutex: %s (%d)", strerror(errno), errno);
    }

#endif
}

Mutex::~Mutex() {
#ifdef WINDOWS
    DeleteCriticalSection(&m_critical_section);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void Mutex::lock() {
    //cout << "Mutex::lock " << &m_mutex << endl;

#ifdef WINDOWS
    EnterCriticalSection(&m_critical_section);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void Mutex::unlock() {
    //cout << "Mutex::unlock " << &m_mutex << endl;

#ifdef WINDOWS
    LeaveCriticalSection(&m_critical_section);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

//
// TimedMutexLock
//
const char *TimedMutexLock::m_classname = "TimedMutexLock";
TimedMutexLock::TimedMutexLock(Mutex &mutex, const char *funcname) :
    m_mutex(mutex),
    m_funcname(funcname) {
    unsigned startTime = Util::getTickCount();
    m_mutex.lock();
    m_acquireTime = Util::getTickCount();

    // Big assumption that this is only used by Objective-C code
    Log::log(0, m_funcname, ChessCore::Log::LEVEL_DEBUG, ChessCore::Log::LANG_OBJC,
             "Mutex %p: waited %umS to lock", &m_mutex, m_acquireTime - startTime);
}

TimedMutexLock::~TimedMutexLock() {
    unsigned endTime = Util::getTickCount();
    // Big assumption that this is only used by Objective-C code
    Log::log(0, m_funcname, ChessCore::Log::LEVEL_DEBUG, ChessCore::Log::LANG_OBJC,
             "Mutex %p: locked for %umS", &m_mutex, endTime - m_acquireTime);
    m_mutex.unlock();
};

}   // namespace ChessCore
