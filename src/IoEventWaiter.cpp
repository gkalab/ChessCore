//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// IOEventWaiter.cpp: IoEvent waiter class definitions.
//

#define VERBOSE_LOGGING 0

#include <ChessCore/IoEventWaiter.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>
#include <string.h>

#ifndef WINDOWS
#include <errno.h>
#endif // !WINDOWS

using namespace std;

namespace ChessCore {
const char *IoEventWaiter::m_classname = "IoEventWaiter";

IoEventWaiter::IoEventWaiter() :
#ifdef WINDOWS
	m_handles()
#else // !WINDOWS
#if IO_EVENT_USE_POLL
    m_fds(),
#elif IO_EVENT_USE_KQUEUE
    m_ioevents(), m_kqueue(-1), m_events(),
#endif // IO_EVENT_USE_xxx
    m_index(0)
#endif // WINDOWS
{

#if IO_EVENT_USE_KQUEUE
    m_kqueue = kqueue();

    if (m_kqueue < 0)
        throw ChessCoreException(Util::format("Failed to create kqueue: %s (%d)",
                                              strerror(errno), errno));
#endif // IO_EVENT_USE_KQUEUE
}

IoEventWaiter::~IoEventWaiter() {
    clearEvents();

#if IO_EVENT_USE_KQUEUE
    if (m_kqueue >= 0) {
        close(m_kqueue);
        m_kqueue = -1;
    }
#endif // IO_EVENT_USE_KQUEUE
}

bool IoEventWaiter::setEvents(const IoEventList &events) {
    //logdbg("numEvents=%u", numEvents);

    bool retval = true;

    clearEvents();

#ifdef WINDOWS

	m_handles.resize(events.size());
	for (unsigned i = 0; i < events.size(); i++)
		m_handles[i] = events[i]->handle();

#else // !WINDOWS

#if IO_EVENT_USE_POLL
    m_fds.resize(events.size());
	for (unsigned i = 0; i < events.size(); i++)
        m_fds[i].fd = events[i]->fd();

#elif IO_EVENT_USE_KQUEUE
    unsigned i;

    for (i = 0; i < events.size(); i++)
        events[i]->lock();

    m_ioevents = &events;
    m_events.resize(events.size());

    for (i = 0; i < numEvents; i++)
        events[i]->initEvent(m_kqueue, &m_events[i]);

    int rv = kevent(m_kqueue, m_events, m_events.size(), 0, 0, 0);

    if (rv == -1) {
        logerr("Error adding events to kqueue: %s (%d)", strerror(errno), errno);
        m_events.clear();
        retval = false;
    } else {
        logdbg("Set %u events", numEvents);
    }

    for (i = 0; i < events.size(); i++)
        events[i]->unlock();
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS

    return retval;
}

void IoEventWaiter::clearEvents() {
#ifdef WINDOWS

	m_handles.clear();

#else // !WINDOWS

#if IO_EVENT_USE_POLL

    m_fds.clear();

#elif IO_EVENT_USE_KQUEUE
    // Remove the events from the kqueue
    if (m_kqueue >= 0 && m_numEvents > 0)
        for (unsigned i = 0; i < m_numEvents; i++)
            m_ioevents[i]->deleteEvent();

    m_ioevents = 0;
    m_events.clear();
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS
}

int IoEventWaiter::wait(int timeout /*=-1*/) {

#ifdef WINDOWS

    for (;;) {
        unsigned startTime = Util::getTickCount();
	    DWORD count = (DWORD)m_handles.size();
        LOGVERBOSE << "Waiting " << timeout << "ms for " << count << " events";
        DWORD waitResult = ::WaitForMultipleObjectsEx(count, &m_handles[0], FALSE, (DWORD)timeout, TRUE);
        if (/*waitResult >= WAIT_OBJECT_0 &&*/waitResult < WAIT_OBJECT_0 + count)
        {
            // Something signalled
            LOGVERBOSE << "Signalled index " << waitResult;
            return (int)waitResult;
        }
        else if (waitResult == WAIT_TIMEOUT)
        {
            LOGVERBOSE << "Time-out";
            return IO_EVENT_WAIT_TIMEOUT;
        }
        else if (waitResult == WAIT_IO_COMPLETION)
        {
            LOGVERBOSE << "I/O completion";
            if (timeout >= 0) {
                // Adjust timeout
                timeout -= (int)(Util::getTickCount() - startTime);
                if (timeout <= 0) {
                    LOGVERBOSE << "Time-out";
                    return IO_EVENT_WAIT_TIMEOUT;
                }
            }
            continue;
        } else {
            LOGWRN << "WaitForMultipleObjects returned 0x" << hex << waitResult <<
                ": " << Util::win32ErrorText(GetLastError());
            return IO_EVENT_WAIT_ERROR;
        }
    }
#else // !WINDOWS

#if IO_EVENT_USE_POLL
    if (m_fds.empty()) {
        logerr("No events registered");
        return IO_EVENT_WAIT_ERROR;
    }

    unsigned i, index;

    for (i = 0; i < m_fds.size(); i++) {
        m_fds[i].events = POLLIN | POLLERR | POLLHUP;
        m_fds[i].revents = 0;
    }

    int pollResult, retval = IO_EVENT_WAIT_ERROR;

    for (;;) {
        nfds_t count = (nfds_t)m_fds.size();
        unsigned startTime = Util::getTickCount();
        LOGVERBOSE << "Waiting " << timeout << "ms for " << count << " events";
        pollResult = ::poll(&m_fds[0], count, timeout);
        if (pollResult == -1 && errno == EINTR) {
            if (timeout >= 0) {         // i.e. not 'infinite'
                // Adjust timeout
                timeout -= (int)(Util::getTickCount() - startTime);
                if (timeout <= 0) {
                    pollResult = IO_EVENT_WAIT_TIMEOUT;
                    break;
                }
            }
            LOGDBG << "Ignored signal interruption";
        } else {
            break;           // A result
        }
    }

    if (pollResult > 0) {
        for (i = 0; i < m_fds.size(); i++) {
            index = (m_index + i) % m_fds.size();

            if (m_fds[index].revents & (POLLERR | POLLHUP)) {
                retval = (m_fds[index].revents & POLLERR) ? IO_EVENT_WAIT_ERROR : IO_EVENT_WAIT_HANGUP;
                break;
            } else if (m_fds[index].revents & POLLIN) {
                retval = index; // index of signalled event
                break;
            }
        }

        m_index = (m_index + 1) % m_fds.size();
    } else if (pollResult == 0)
        retval = IO_EVENT_WAIT_TIMEOUT;
    else { // pollResult < 0
        LOGWRN << "poll() failed: " << strerror(errno) << " (" << errno << ")";
        retval = IO_EVENT_WAIT_ERROR;
    }

    return retval;

#elif IO_EVENT_USE_KQUEUE

	if (m_events.empty()) {
        logerr("No events registered");
        return IO_EVENT_WAIT_ERROR;
    }

    int keventResult, retval = IO_EVENT_WAIT_ERROR;
    vector<struct kevent> (events.size());

    for (;; ) {
        struct timespec ts;

        if (timeout >= 0) {
            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (long)(timeout % 1000) * 1000000L;
        }

        unsigned startTime = Util::getTickCount();
        keventResult = kevent(m_kqueue, 0, 0, &events[0], events.size(), timeout == -1 ? 0 : &ts);

        if (keventResult == -1 && errno == EINTR) {
            unsigned elapsed = Util::getTickCount() - startTime;

            if (timeout >= 0) {
                if (elapsed >= (unsigned)timeout) {
                    keventResult = IO_EVENT_WAIT_TIMEOUT;
                    break; // A result (timeout)
                }

                timeout -= (int)elapsed; // Ignore EINTR and carry on
            } // else timeout == -1 (infinite) so leave it as is

            LOGDBG << "Ignored signal interruption";
        } else {
            break;           // A result
        }
    }

    if (keventResult > 0) {
        unsigned index = m_index;

        for (int i = 0; i < keventResult; i++) {
            index = (m_index + i) % keventResult;

            if (events[index].flags & EV_ERROR) {
                retval = IO_EVENT_WAIT_ERROR;
                break;
            } else if (events[index].flags & EV_EOF) {
                retval = IO_EVENT_WAIT_HANGUP;
                break;
            } else {
                // Find the index of this event in m_events
                unsigned j;

                for (j = 0; j < events.size(); j++)
                    if (m_events[j].ident == events[index].ident &&
                        m_events[j].filter == events[index].filter)
                        break;

                if (j == events.size()) {
                    logerr("Failed to find event %u/%u", events[i].ident, events[i].filter);
                    retval = IO_EVENT_WAIT_ERROR;
                } else {
                    retval = j;
                }

                break;
            }
        }

        m_index = (m_index + 1) % m_keventResult;
    } else if (keventResult == 0)
        retval = IO_EVENT_WAIT_TIMEOUT;
    else { // keventResult < 0
        LOGWRN << "kevent() failed: " << strerror(errno) << " (" << errno << ")";
        retval = IO_EVENT_WAIT_ERROR;
    }

    return retval;
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS
}
}   // namespace ChessCore
