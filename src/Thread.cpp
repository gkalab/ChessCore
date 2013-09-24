//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Thread.cpp: Thread class implementation.
//

#include <ChessCore/Thread.h>
#include <ChessCore/Log.h>
#include <string.h>
#include <errno.h>

#ifdef WINDOWS
#include <process.h>
#endif // WINDOWS

using namespace std;

namespace ChessCore {

const char *Thread::m_classname = "Thread";

// Thread entry point trampoline function (C -> C++)
THREAD_TRAMPOLINE_RETURN threadTrampoline(void *p) {
    Thread *obj = static_cast<Thread *>(p);
    obj->m_threadRunning = true;
    obj->entry();
    obj->m_threadRunning = false;

#ifdef WINDOWS
    obj->m_threadHandle = INVALID_HANDLE_VALUE;
#else
    obj->m_threadId = 0;
    return 0; // pthread expects a return
#endif
}

Thread::Thread(void) {
#ifdef WINDOWS
    m_threadHandle = INVALID_HANDLE_VALUE;
#else
    m_threadId = 0;
#endif

    m_threadRunning = false;
}

Thread::~Thread(void) {
}

bool Thread::start() {

#ifdef WINDOWS

    m_threadHandle = (HANDLE)_beginthread(threadTrampoline, 0, this);
    if (m_threadHandle == INVALID_HANDLE_VALUE) {
        logerr("Failed to create thread: %s (%d)", strerror(errno), errno);
        return false;
    }

#else // !WINDOWS

	if (pthread_create(&m_threadId, 0, threadTrampoline, this) < 0) {
        logerr("Failed to create thread: %s (%d)", strerror(errno), errno);
        return false;
    }

#endif // WINDOWS

	return true;
}

}   // namespace ChessCore

