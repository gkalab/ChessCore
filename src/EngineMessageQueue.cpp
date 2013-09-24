//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// EngineMessageQueue.h: Engine Input/Output Queue class implementation.
//

#include <ChessCore/EngineMessageQueue.h>
#include <ChessCore/EngineMessage.h>
#include <ChessCore/Log.h>

#define ENGINE_MESSAGE_DEBUGGING 0

using namespace std;

namespace ChessCore {
const char *EngineMessageQueue::m_classname = "EngineMessageQueue";

EngineMessageQueue::EngineMessageQueue() :
    m_mutex(),
    m_event(),
    m_queue() {
}

EngineMessageQueue::~EngineMessageQueue() {
    clear();
}

size_t EngineMessageQueue::count() {
    MutexLock lock(m_mutex);
    return m_queue.size();
}

void EngineMessageQueue::clear() {
    MutexLock lock(m_mutex);
    std::shared_ptr<EngineMessage> message;

    do
        message = dequeue();
    while (message.get() != 0);

    m_event.reset();
}

bool EngineMessageQueue::enqueue(std::shared_ptr<EngineMessage> message) {
    MutexLock lock(m_mutex);
    m_queue.push(message);

#if ENGINE_MESSAGE_DEBUGGING
    logdbg("Sending message '%s'. size=%u", EngineMessage::typeDesc(message->type), m_queue.size());
#endif

    if (m_queue.size() == 1)
        m_event.set();

    return true;
}

std::shared_ptr<EngineMessage> EngineMessageQueue::dequeue() {
    MutexLock lock(m_mutex);

    shared_ptr<EngineMessage> message;
    if (!m_queue.empty()) {
        message = m_queue.front();
        m_queue.pop();

#if ENGINE_MESSAGE_DEBUGGING
        logdbg("Received message '%s'. size=%u", EngineMessage::typeDesc(message->type), m_queue.size());
#endif
    }

    if (m_queue.empty())
        m_event.reset();

    return message;
}

std::shared_ptr<EngineMessage> EngineMessageQueue::dequeue(int timeout) {
    std::shared_ptr<EngineMessage> message(dequeue());
    if (message.get() != 0)
        return message; // Already something waiting

    IoEventWaiter waiter;
    IoEventList e(1);
    e[0] = &m_event;

    if (waiter.setEvents(e) && waiter.wait(timeout) == 0)
        message = dequeue();

    return message;
}
}   // namespace ChessCore
