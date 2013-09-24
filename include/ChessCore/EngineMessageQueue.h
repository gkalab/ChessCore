//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// EngineMessageQueue.h: Engine Input/Output Queue class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Mutex.h>
#include <ChessCore/EngineMessage.h>
#include <ChessCore/IoEventWaiter.h>
#include <queue>
#include <memory>

namespace ChessCore {
class CHESSCORE_EXPORT EngineMessageQueue {
private:
    static const char *m_classname;

protected:
    mutable Mutex m_mutex;
    IoEvent m_event;
    std::queue<std::shared_ptr<EngineMessage>> m_queue;

public:
    EngineMessageQueue();
    virtual ~EngineMessageQueue();

    /**
     * Get the number of messages on the queue.
     *
     * @return The number of messages on the queue.
     */
    size_t count();

    /**
     * Clear the queue, removing all messages.
     */
    void clear();

    /**
     * Enqueue a message.
     *
     * @param message The message to add to the queue.
     *
     * @return true if the message was queued successfully, else false.
     */
    bool enqueue(std::shared_ptr<EngineMessage> message);

    /**
     * Dequeue a message.
     *
     * @return The message, if one was available.  Test if a message was dequeued
     * by calling the shared_ptr::get() method and testing for non-zero.
     */
    std::shared_ptr<EngineMessage> dequeue();

    /**
     * Wait for a message to appear in the queue and then dequeue it.
     *
     * @param timeout The time to wait for a message to appear (milliseconds).  -1 for infinite.
     *
     * @return The message, if one was available.  Test if a message was dequeued
     * by calling the shared_ptr::get() method and testing for non-zero.
     */
    std::shared_ptr<EngineMessage> dequeue(int timeout);

    /**
     * Get the IOEvent object associated with the queue, which can be used to wait for
     * activity using IOEventWaiter.
     *
     * @return The IOEvent object.
     */
    IoEvent &event() {
        return m_event;
    }
};
} // namespace ChessCore
