#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace event_bus::detail
{

template <typename EVENT>
concept PlainEvent = std::same_as<EVENT, std::remove_cvref_t<EVENT>>;

class SubscriberStorage
{
public:
    SubscriberStorage() = default;

    SubscriberStorage(SubscriberStorage const&) = delete;
    SubscriberStorage& operator=(SubscriberStorage const&) = delete;

    SubscriberStorage(SubscriberStorage&& other) noexcept(false);
    SubscriberStorage& operator=(SubscriberStorage&& other) noexcept(false);

    template <PlainEvent EVENT, typename CALLBACK>
        requires std::is_nothrow_invocable_r_v<void, CALLBACK&, EVENT const&>
    void subscribe(CALLBACK&& callback);

    template <PlainEvent EVENT>
    void publish(EVENT const& event) noexcept;

    void clear();

private:
    inline static std::size_t m_eventIndex = 0;

    template <PlainEvent EVENT>
    static std::size_t eventIndex() noexcept;

private:
    std::size_t m_publishCount{0};

    struct PublishGuard
    {
        explicit PublishGuard(SubscriberStorage& s) noexcept;
        ~PublishGuard() noexcept;

        SubscriberStorage& storage;
    };

    void throwIfPublishing() const;

private:
    void reserveAndResizeIfNecessary(std::size_t index);

    using erased_callback_t = std::move_only_function<void(void const*) noexcept>;
    std::vector<std::vector<erased_callback_t>> m_callbacks;
};

// ------------------------------------------------------------------

inline SubscriberStorage::SubscriberStorage(SubscriberStorage&& other) noexcept(false)
{
    other.throwIfPublishing();
    m_callbacks = std::move(other.m_callbacks);
}

inline SubscriberStorage& SubscriberStorage::operator=(SubscriberStorage&& other) noexcept(false)
{
    if (this == &other)
        return *this;

    throwIfPublishing();
    other.throwIfPublishing();

    m_callbacks = std::move(other.m_callbacks);
    return *this;
}

template <PlainEvent EVENT, typename CALLBACK>
    requires std::is_nothrow_invocable_r_v<void, CALLBACK&, EVENT const&>
void SubscriberStorage::subscribe(CALLBACK&& callback)
{
    throwIfPublishing();

    if constexpr (requires { static_cast<bool>(callback); })
    {
        if (!static_cast<bool>(callback))
            throw std::invalid_argument("callback is empty");
    }

    std::size_t const index = eventIndex<EVENT>();
    reserveAndResizeIfNecessary(index);

    auto typeErasedCb = [cb = std::forward<CALLBACK>(callback)](void const* event) mutable noexcept { std::invoke(cb, *static_cast<EVENT const*>(event)); };

    m_callbacks[index].emplace_back(std::move(typeErasedCb));
}

template <PlainEvent EVENT>
void SubscriberStorage::publish(EVENT const& event) noexcept
{
    std::size_t const index = eventIndex<EVENT>();
    if (index >= m_callbacks.size())
        return;

    PublishGuard guard(*this);
    for (auto& callback : m_callbacks[index])
    {
        callback(std::addressof(event));
    }
}

inline void SubscriberStorage::clear()
{
    throwIfPublishing();
    m_callbacks.clear();
}

template <PlainEvent EVENT>
std::size_t SubscriberStorage::eventIndex() noexcept
{
    static std::size_t const index = m_eventIndex++;
    return index;
}

inline SubscriberStorage::PublishGuard::PublishGuard(SubscriberStorage& s) noexcept : storage(s)
{
    ++storage.m_publishCount;
}
inline SubscriberStorage::PublishGuard::~PublishGuard() noexcept
{
    --storage.m_publishCount;
}

inline void SubscriberStorage::throwIfPublishing() const
{
    if (m_publishCount != 0)
    {
        throw std::logic_error("EventBus modification during publish is forbidden");
    }
}

inline void SubscriberStorage::reserveAndResizeIfNecessary(std::size_t index)
{
    if (index >= m_callbacks.size())
    {
        if (index >= m_callbacks.capacity())
        {
            m_callbacks.reserve(index * 2);
        }

        // +1, to be able to access m_callbacks[index]
        m_callbacks.resize(index + 1);
    }
}

} // namespace event_bus::detail
