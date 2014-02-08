//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Mutex.h: Mutex class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>

#ifndef WINDOWS
#include <pthread.h>
#endif // !WINDOWS

namespace ChessCore {

class CHESSCORE_EXPORT Mutex {
protected:
#ifdef WINDOWS
    CRITICAL_SECTION m_critical_section;
#else
    pthread_mutex_t m_mutex;
#endif

public:
    Mutex();
    virtual ~Mutex();
    void lock();
    bool tryLock();
    void unlock();
};

//
// RAII-style mutex locker
//
class MutexLock {
protected:
    Mutex &m_mutex;
public:
    MutexLock(Mutex &mutex) :
        m_mutex(mutex) {
        m_mutex.lock();
    }

    inline ~MutexLock() {
        m_mutex.unlock();
    }
};

//
// RAII-style try-mutex locker
//
class MutexTryLock {
protected:
    Mutex *m_mutex;
    bool m_locked;
public:
    MutexTryLock(Mutex &mutex) :
        m_mutex(&mutex) {
        if (m_mutex)
            m_locked = m_mutex->tryLock();
    }

    MutexTryLock(Mutex *mutex) :
        m_mutex(mutex) {
        if (m_mutex)
            m_locked = m_mutex->tryLock();
    }

    inline ~MutexTryLock() {
        if (m_mutex && m_locked)
            m_mutex->unlock();
    }

    bool isLocked() const {
        return m_locked;
    }
};

//
// RAII-style mutex locker with timing support
//
class CHESSCORE_EXPORT TimedMutexLock {
protected:
    static const char *m_classname;
    Mutex &m_mutex;
    const char *m_funcname;
    unsigned m_acquireTime;
public:
    TimedMutexLock(Mutex &mutex, const char *funcname);
    ~TimedMutexLock();
};

#if TIMED_MUTEX
#define MUTEX_LOCK(m) ChessCore::TimedMutexLock lock(m, __FUNCTION__)
#else
#define MUTEX_LOCK(m) ChessCore::MutexLock lock(m)
#endif

    //
    // Simple RAII class to set a boolean and clear it on destruction while holding a mutex
    //
    class MutexLockWithBool {
    private:
        Mutex &m_mutex;
        bool &m_b;
    public:
        MutexLockWithBool(Mutex &mutex, bool &b) :
            m_mutex(mutex),
            m_b(b)
        {
            m_mutex.lock();
            m_b = true;
        }
        ~MutexLockWithBool() {
            m_b = false;
            m_mutex.unlock();
        }
    };
    


} // namespace ChessCore
