#pragma once

#include "detail/SubscribersStorage.hpp"

#include <deque>
#include <functional>
#include <utility>

namespace event_bus
{

class AsyncEventBus
{
public:
    template <detail::PlainEvent EVENT, typename CALLBACK>
    void subscribe(CALLBACK&& callback);

    template <detail::PlainEvent EVENT>
    void publish(EVENT event);

    void dispatch();

    void clear();

private:
    detail::SubscriberStorage m_storage;
    std::deque<std::move_only_function<void(detail::SubscriberStorage&) noexcept>> m_eventPublishers;
};

// ------------------------------------------------------------------

template <detail::PlainEvent EVENT, typename CALLBACK>
void AsyncEventBus::subscribe(CALLBACK&& callback)
{
    m_storage.subscribe<EVENT>(std::forward<CALLBACK>(callback));
}

template <detail::PlainEvent EVENT>
void AsyncEventBus::publish(EVENT event)
{
    auto eventPublisher = [event = std::move(event)](detail::SubscriberStorage& subscribers) mutable noexcept { subscribers.publish(event); };

    m_eventPublishers.emplace_back(std::move(eventPublisher));
}

inline void AsyncEventBus::dispatch()
{
    while (!m_eventPublishers.empty())
    {
        auto event = std::move(m_eventPublishers.front());
        m_eventPublishers.pop_front();

        event(m_storage);
    }
}

inline void AsyncEventBus::clear()
{
    m_storage.clear();
    m_eventPublishers.clear();
}

} // namespace event_bus
