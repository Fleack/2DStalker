#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace network
{
struct Config
{
    std::uint32_t maxMessageBytes{1024 * 1024};
    std::size_t maxQueuedMessages{1024};
    std::size_t maxPendingRequests{1024};
    std::chrono::steady_clock::duration connectTimeout{std::chrono::seconds{5}};
    std::chrono::steady_clock::duration requestTimeout{std::chrono::seconds{5}};

    void validate() const
    {
        if (this->maxMessageBytes == 0)
        {
            throw std::invalid_argument{"Client maxMessageBytes must be greater than zero"};
        }

        if (this->maxQueuedMessages == 0)
        {
            throw std::invalid_argument{"Client maxQueuedMessages must be greater than zero"};
        }

        if (this->maxPendingRequests == 0)
        {
            throw std::invalid_argument{"Client maxPendingRequests must be greater than zero"};
        }

        if (this->connectTimeout <= std::chrono::steady_clock::duration::zero())
        {
            throw std::invalid_argument{"Client connectTimeout must be greater than zero"};
        }

        if (this->requestTimeout <= std::chrono::steady_clock::duration::zero())
        {
            throw std::invalid_argument{"Client requestTimeout must be greater than zero"};
        }
    }
};

enum class ConnectionState : std::uint8_t
{
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
};
} // namespace network
