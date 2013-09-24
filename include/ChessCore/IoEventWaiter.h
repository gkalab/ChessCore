//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// IOEventWaiter.h: IoEvent waiter class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/IoEvent.h>

#ifndef WINDOWS
#include <poll.h>
#endif // !WINDOWS
#include <vector>

namespace ChessCore {
typedef std::vector<IoEvent *>   IoEventList;

#define IO_EVENT_WAIT_ERROR   -1
#define IO_EVENT_WAIT_HANGUP  -2
#define IO_EVENT_WAIT_TIMEOUT -3

class CHESSCORE_EXPORT IoEventWaiter {
private:
    static const char *m_classname;

protected:

#ifdef WINDOWS

	std::vector<HANDLE> m_handles;

#else // !WINDOWS

#if IO_EVENT_USE_POLL

    std::vector<pollfd> m_fds;

#elif IO_EVENT_USE_KQUEUE

	IoEventList * m_ioevents;
    std::vector<struct kevent> m_events;
    int m_kqueue;

#endif // IO_EVENT_USE_xxx

    unsigned m_index;       // Round-robin signalled events

#endif // WINDOWS

public:
    IoEventWaiter();
    virtual ~IoEventWaiter();

    unsigned numEvents() const {
#ifdef WINDOWS
		return (unsigned)m_handles.size();
#else // !WINDOWS
#if IO_EVENT_USE_POLL
        return (unsigned)m_fds.size();
#elif IO_EVENT_USE_KQUEUE
        return (unsigned)m_events.size();
#endif // IO_EVENT_USE_POLL
#endif // WINDOWS
    }

    /**
     * Register events to wait for.
     *
     * @param events Vector of IoEvent pointers.
     *
     * @return true if the events were registered successfully, else false.
     */
    bool setEvents(const IoEventList &events);

    /**
     * Unregister events to wait for.
     */
    void clearEvents();

    /**
     * Add an event to wait for, at the end of the list.
     *
     * @param event The event to add.
     *
     * @return true if the event was added successfully.
     */
    bool addEvent(IoEvent *event);

    /**
     * Remove an event to wait for.
     *
     * @param event The event to remove.
     *
     * @return true if the event was removed successfully.
     */
    bool removeEvent(IoEvent *event);

    /**
     * Wait for an event to be signalled.
     *
     * @param timeout Number of milliseconds to wait before timeout. -1 for infinite.
     *
     * @return The index of the event that was signalled (0 onwards), or a
     * IO_EVENT_WAIT_xxx constant if something else happened.
     */
    int wait(int timeout = -1);
};
} // namespace ChessCore
