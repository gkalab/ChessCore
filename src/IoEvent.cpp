//
// ChessCore (c)2008-2009 Andy Duplain <trojanfoe@gmail.com>
//
// IoEvent.cpp: IoEvent class implementation.
//

#include <ChessCore/IoEvent.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>

#ifndef WINDOWS
#include <string.h>
#include <unistd.h>
#include <errno.h>
#endif // !WINDOWS

#if IO_EVENT_USE_POLL
#include <fcntl.h>
#include <poll.h>
#endif // IO_EVENT_USE_POLL

using namespace std;

namespace ChessCore {
const char *IoEvent::m_classname = "IoEvent";

#if IO_EVENT_USE_KQUEUE
unsigned IoEvent::m_userIdents = 0;
#endif // IO_EVENT_USE_KQUEUE

#ifdef WINDOWS

IoEvent::IoEvent() {
	m_handle = CreateEvent(0, TRUE, FALSE, 0);
	if (!m_handle) {
		throw ChessCoreException(string("Failed to create Event object: ") + Util::win32ErrorText(GetLastError()));
    }
}

IoEvent::IoEvent(HANDLE handle) {
    if (!Util::win32DuplicateHandle(handle, m_handle)) {
        throw ChessCoreException(string("Failed to duplicate event handle: ") + Util::win32ErrorText(GetLastError()));
    }
}

#else // !WINDOWS

#if IO_EVENT_USE_POLL

IoEvent::IoEvent() : 
	m_ownsFDs(true) {

    if (pipe(m_pipe) < 0)
        throw ChessCoreException("Failed to create pipe: %s (%d)", strerror(errno), errno);

    if (fcntl(m_pipe[0], F_SETFL, O_NONBLOCK) < 0)
        throw ChessCoreException("Failed to set pipe non-blocking mode: %s (%d)", strerror(errno), errno);
}

IoEvent::IoEvent(int fd) :
	m_ownsFDs(false) {

    m_pipe[0] = fd;
    m_pipe[1] = -1;
}

#elif IO_EVENT_USE_KQUEUE

IoEvent::IoEvent() : 
	m_mutex(),
	m_type(IoEventTypeUser),
	m_ident(m_userIdents++),
	m_kqueue(0) {
}

IoEvent::IoEvent(int fd) :
	m_mutex(),
	m_type(IoEventTypeFile),
	m_ident(fd), m_kqueue(0) {
}

#endif // IO_EVENT_USE_xxx

#endif // WINDOWS

IoEvent::~IoEvent() {

#ifdef WINDOWS
	if (m_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(m_handle);
		m_handle = NULL;
	}
#else // !WINDOWS

#if IO_EVENT_USE_POLL
    if (m_pipe[0] >= 0) {
        if (m_ownsFDs)
            close(m_pipe[0]);

        m_pipe[0] = -1;
    }

    if (m_pipe[1] >= 0) {
        if (m_ownsFDs)
            close(m_pipe[1]);

        m_pipe[1] = -1;
    }

#elif IO_EVENT_USE_KQUEUE
    logdbg("%p", this);

    deleteEvent();
    m_type = IoEventTypeNone;
    m_ident = -1;
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS
}

void IoEvent::set() {

#ifdef WINDOWS

	SetEvent(m_handle);

#else // !WINDOWS

#if IO_EVENT_USE_POLL
    if (m_ownsFDs)
        write(m_pipe[1], "x", 1);

#elif IO_EVENT_USE_KQUEUE
    MutexLock lock(m_mutex);

    if (m_type == IoEventTypeUser && m_kqueue >= 0) {
        logdbg("%p", this);

        struct kevent kev[1];
        EV_SET(&kev[0], m_ident, EVFILT_USER, 0, NOTE_TRIGGER, 0, 0);
        int rv = kevent(m_kqueue, kev, 1, 0, 0, 0);

        if (rv == -1)
            logerr("Failed to set event: %s (%d)", strerror(errno), errno);
    }
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS
}

void IoEvent::reset() {

#ifdef WINDOWS

	ResetEvent(m_handle);

#else // !WINDOWS

#if IO_EVENT_USE_POLL
    if (m_ownsFDs) {
        uint8_t buf;

        while (read(m_pipe[0], &buf, 1) == 1)
            ;
    }

#elif IO_EVENT_USE_KQUEUE
    MutexLock lock(m_mutex);

    if (m_type == IoEventTypeUser && m_kqueue >= 0) {
        logdbg("%p", this);

        struct kevent kev[1];
        EV_SET(&kev[0], m_ident, EVFILT_USER, EV_CLEAR, 0, 0, 0);
        int rv = kevent(m_kqueue, kev, 1, 0, 0, 0);

        if (rv == -1)
            logerr("Failed to reset event: %s (%d)", strerror(errno), errno);
    }
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS
}

#if IO_EVENT_USE_KQUEUE
void IoEvent::initEvent(int kqueue, struct kevent *kevent) {
    m_kqueue = kqueue;

    if (kevent) {
        logdbg("%p", this);

        if (m_type == IoEventTypeFile)
            // File descriptor event
            EV_SET(kevent, m_ident, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
        else if (m_type == IoEventTypeUser)
            // User event
            EV_SET(kevent, m_ident, EVFILT_USER, EV_ADD | EV_ENABLE, NOTE_TRIGGER, 0, 0);
    }
}

void IoEvent::deleteEvent() {
    MutexLock lock(m_mutex);

    if (m_kqueue >= 0) {
        struct kevent kev[1];

        if (m_type == IoEventTypeFile)
            // File descriptor event
            EV_SET(&kev[0], m_ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
        else if (m_type == IoEventTypeUser)
            // User event
            EV_SET(&kev[0], m_ident, EVFILT_USER, EV_DELETE, 0, 0, 0);

        int rv = kevent(m_kqueue, kev, 1, 0, 0, 0);

        if (rv == -1)
            logerr("Failed to delete event: %s (%d)", strerror(errno), errno);

        m_kqueue = -1;
    }
}

void IoEvent::lock() {
    m_mutex.lock();
}

void IoEvent::unlock() {
    m_mutex.unlock();
}
#endif // IO_EVENT_USE_KQUEUE
}   // namespace ChessCore
