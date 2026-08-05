#pragma once

#include "detail/SubscribersStorage.hpp"

#include <utility>

namespace event_bus
{

class SyncEventBus
{
public:
    template <detail::PlainEvent EVENT, typename CALLBACK>
    void subscribe(CALLBACK&& callback);

    template <detail::PlainEvent EVENT>
    void publish(EVENT const& event) noexcept;

    void clear();

private:
    detail::SubscriberStorage m_storage;
};

// ------------------------------------------------------------------

template <detail::PlainEvent EVENT, typename CALLBACK>
void SyncEventBus::subscribe(CALLBACK&& callback)
{
    m_storage.subscribe<EVENT>(std::forward<CALLBACK>(callback));
}

template <detail::PlainEvent EVENT>
void SyncEventBus::publish(EVENT const& event) noexcept
{
    m_storage.publish<EVENT>(event);
}

inline void SyncEventBus::clear()
{
    m_storage.clear();
}

} // namespace event_bus
