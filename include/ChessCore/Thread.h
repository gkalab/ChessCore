//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Thread.h: Thread class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>

#ifndef WINDOWS
#include <pthread.h>
#endif // !WINDOWS

namespace ChessCore {

#ifdef WINDOWS
#define THREAD_TRAMPOLINE_RETURN void
#else
#define THREAD_TRAMPOLINE_RETURN void *
#endif

extern "C" THREAD_TRAMPOLINE_RETURN threadTrampoline(void *p);

class CHESSCORE_EXPORT Thread {
protected:
    static const char *m_classname;
    friend THREAD_TRAMPOLINE_RETURN threadTrampoline(void *p);

#ifdef WINDOWS
    HANDLE m_threadHandle;
#else
    pthread_t m_threadId;
#endif

    bool m_threadRunning;

public:
    Thread();
    virtual ~Thread();

    /**
     * Start a new thread and call the derived entry() method.
     */
    bool start();

#ifdef WINDOWS

    inline HANDLE threadHandle() const {
        return m_threadHandle;
    }

    /**
     * Get the thread identifier.
     *
     * @return The thread identifier.
     */
    DWORD threadId();

    /**
     * Get the thread identifier of the calling thread.
     *
     * @return The thread identifier of the calling thread.
     */
    static DWORD currentThreadId() {
        return ::GetCurrentThreadId();
    }

#else // !WINDOWS

    /**
     * Get the thread identifier.
     *
     * @return The thread identifier.
     */
    inline pthread_t threadId() const {
        return m_threadId;
    }

    /**
     * Get the thread identifier of the calling thread.
     *
     * @return The thread identifier of the calling thread.
     */
    static pthread_t currentThreadId() {
        return ::pthread_self();
    }

#endif // WINDOWS

    inline bool isThreadRunning() const {
        return m_threadRunning;
    }

protected:
    /**
     * Thread entry point
     */
    virtual void entry() = 0;
};

} // namespace ChessCore
