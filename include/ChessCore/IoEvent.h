//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// IOEvent.h: IoEvent class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Mutex.h>

#if IO_EVENT_USE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif // IO_EVENT_USE_KQUEUE

namespace ChessCore {
class CHESSCORE_EXPORT IoEvent {
private:
    static const char *m_classname;

protected:

#ifdef WINDOWS
	HANDLE m_handle;
#else // !WINDOWS

#if IO_EVENT_USE_POLL
    int m_pipe[2];
    bool m_ownsFDs;

#elif IO_EVENT_USE_KQUEUE
    static unsigned m_userIdents;

    enum Type {
        IoEventTypeNone,
        IoEventTypeFile,
        IoEventTypeUser
    };

    Mutex m_mutex;
    Type m_type;
    int m_ident;
    int m_kqueue;
#endif // IO_EVENT_USE_xxx
#endif // WINDOWS

public:

    IoEvent();              // Creates a user event

#ifdef WINDOWS
    IoEvent(HANDLE handle);
#else
    IoEvent(int fd);        // Create a file event
#endif // !WINDOWS

    virtual ~IoEvent();

    /**
     * Set the event to signalled state.
     */
    void set();

    /**
     * Reset the event from signalled state.
     */
    void reset();

#ifdef WINDOWS
	HANDLE handle() const {
		return m_handle;
	}

#else // !WINDOWS

#if IO_EVENT_USE_POLL
    inline int fd() const {
        return m_pipe[0];
    }

#elif IO_EVENT_USE_KQUEUE
    inline Type type() const {
        return m_type;
    }

    inline int ident() const {
        return m_ident;
    }

    void initEvent(int kqueue, struct kevent *kevent);
    void deleteEvent();
    void lock();
    void unlock();
#endif // IO_EVENT_USE_POLL
#endif // WINDOWS
};

} // namespace ChessCore
