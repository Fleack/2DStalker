#pragma once

#include "shared/logger/logger.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/network/NetworkSide.hpp"
#include "shared/network/connection_id.hpp"
#include "shared/protocol/message.pb.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/post.hpp>
#include <asio/strand.hpp>

namespace s2d::network
{

template <ANetworkSide NetworkSide>
class ConnectionWriter
{
public:
    using outcoming_message_t = NetworkSide::outcoming_message_t;
    using lifetime_guard_t = std::shared_ptr<void>;

    ConnectionWriter(
        asio::ip::tcp::socket& socket,
        connection_id id,
        std::uint32_t max_message_bytes,
        std::function<void()> onError)
        : m_strand{socket.get_executor()}
        , m_messageChannel{max_message_bytes}
        , m_onError{std::move(onError)}
        , m_socket{socket}
        , m_id{id}
    {}

    void send(outcoming_message_t message, lifetime_guard_t connection)
    {
        asio::post(
            m_strand,
            [this, message = std::move(message), connection = std::move(connection)]() mutable {
                enqueue(std::move(message), std::move(connection));
            });
    }

private:
    void enqueue(outcoming_message_t message, lifetime_guard_t connection)
    {
        if (!m_socket.is_open())
        {
            LOG(warn, "Connection[id={}] is stopped or socket is closed", m_id.id);
            return;
        }

        m_queue.push_back(std::move(message));

        if (m_writeInProgress)
        {
            return;
        }

        m_writeInProgress = true;
        asio::co_spawn(
            m_strand,
            [this, connection = std::move(connection)]() mutable -> asio::awaitable<void> {
                co_await drain(std::move(connection));
            },
            asio::detached);
    }

    asio::awaitable<void> drain(lifetime_guard_t connection)
    {
        while (!m_queue.empty())
        {
            auto message = std::move(m_queue.front());
            m_queue.pop_front();

            try
            {
                LOG(debug, "Sending to client[id={}] message[id={}] with status {}", m_id.id, message.request_id(), std::to_underlying(message.status())); // TODO: improve logging
                co_await m_messageChannel.writeMessage(m_socket, message);
            }
            catch (std::exception const& e)
            {
                LOG(warn, "Connection[id={}] error during write: {}", m_id.id, e.what());
                m_queue.clear();
                m_writeInProgress = false;
                m_onError();
                co_return;
            }
        }

        m_writeInProgress = false;
        (void)connection;
        co_return;
    }

private:
    std::deque<outcoming_message_t> m_queue;
    asio::strand<asio::any_io_executor> m_strand;
    MessageChannel m_messageChannel;
    std::function<void()> m_onError;
    asio::ip::tcp::socket& m_socket;
    connection_id m_id;
    bool m_writeInProgress{false};
};

} // namespace s2d::network
