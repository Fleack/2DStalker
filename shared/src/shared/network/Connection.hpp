#pragma once

#include "shared/logger/logger.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/network/connection_id.hpp"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>

namespace s2d::network
{
using namespace boost;

template <typename IncomingMessage, typename OutgoingMessage>
class Connection : public std::enable_shared_from_this<Connection<IncomingMessage, OutgoingMessage>>
{
public:
    using incoming_message_t = IncomingMessage;
    using outgoing_message_t = OutgoingMessage;

    using message_handler_t = std::move_only_function<asio::awaitable<void>(incoming_message_t)>;
    using close_handler_t = std::move_only_function<void()>;

    struct Config
    {
        std::uint32_t maxMessageBytes{};
        std::size_t maxQueuedMessages{1024};
    };

    static std::shared_ptr<Connection> create(
        connection_id id,
        asio::ip::tcp::socket&& socket,
        Config config,
        message_handler_t onMessage,
        close_handler_t onClosed = {})
    {
        // Can not use std::make_shared, due to private constructor
        return std::shared_ptr<Connection>{
            new Connection{
                id,
                std::move(socket),
                config,
                std::move(onMessage),
                std::move(onClosed)}};
    }

    void start()
    {
        auto weak = this->weak_from_this();

        asio::post(
            m_strand,
            [weak] {
                if (auto self = weak.lock())
                {
                    self->startImpl();
                }
            });
    }

    void stop() noexcept
    {
        try
        {
            auto weak = this->weak_from_this();

            asio::post(
                m_strand,
                [weak] {
                    if (auto self = weak.lock())
                    {
                        self->stopImpl("local stop");
                    }
                });
        }
        catch (std::exception const& e)
        {
            LOG(warn, "Connection[id={}] stop failed: {}", m_id.id, e.what());
        }
    }

    void send(outgoing_message_t message)
    {
        auto weak = this->weak_from_this();

        asio::post(
            m_strand,
            [weak, message = std::move(message)]() mutable {
                if (auto self = weak.lock())
                {
                    self->sendImpl(std::move(message));
                }
            });
    }

    [[nodiscard]] connection_id getId() const noexcept
    {
        return m_id;
    }

private:
    Connection(
        connection_id id,
        asio::ip::tcp::socket&& socket,
        Config config,
        message_handler_t onMessage,
        close_handler_t onClosed)
        : m_socket{std::move(socket)}
        , m_strand{m_socket.get_executor()}
        , m_channel{config.maxMessageBytes}
        , m_onMessage{std::move(onMessage)}
        , m_onClosed{std::move(onClosed)}
        , m_id{id}
        , m_remoteEndpoint{makeRemoteEndpointString(m_socket)}
        , m_maxQueuedMessages{config.maxQueuedMessages}
    {
        if (!m_onMessage)
        {
            throw std::invalid_argument{"Connection requires message handler"};
        }

        if (config.maxMessageBytes == 0)
        {
            throw std::invalid_argument{"Connection maxMessageBytes must be greater than zero"};
        }

        if (config.maxQueuedMessages == 0)
        {
            throw std::invalid_argument{"Connection maxQueuedMessages must be greater than zero"};
        }
    }

    void startImpl()
    {
        if (m_started)
        {
            LOG(warn, "Connection[id={}] already started", m_id.id);
            return;
        }

        if (m_closed)
        {
            LOG(warn, "Connection[id={}] cannot start closed connection", m_id.id);
            return;
        }

        m_started = true;

        LOG(info, "Connection[id={}] started with {}", m_id.id, m_remoteEndpoint);

        auto self = this->shared_from_this();

        asio::co_spawn(
            m_strand,
            [self]() -> asio::awaitable<void> {
                co_await self->readLoop();
            },
            asio::detached);
    }

    void sendImpl(outgoing_message_t message)
    {
        if (m_closed)
        {
            LOG(warn, "Connection[id={}] is closed, outgoing message ignored", m_id.id);
            return;
        }

        if (m_writeQueue.size() >= m_maxQueuedMessages)
        {
            LOG(warn, "Connection[id={}] write queue limit exceeded", m_id.id);
            stopImpl("write queue limit exceeded");
            return;
        }

        m_writeQueue.push_back(std::move(message));

        if (m_writing)
        {
            return;
        }

        m_writing = true;

        auto self = this->shared_from_this();

        asio::co_spawn(
            m_strand,
            [self]() -> asio::awaitable<void> {
                co_await self->writeLoop();
            },
            asio::detached);
    }

    asio::awaitable<void> readLoop()
    {
        while (!m_closed)
        {
            try
            {
                auto message = co_await m_channel.readMessage<incoming_message_t>(m_socket);

                if (m_closed)
                {
                    co_return;
                }

                LOG(debug, "Received message from connection[id={}]", m_id.id);

                try
                {
                    co_await m_onMessage(std::move(message));
                }
                catch (std::exception const& e)
                {
                    LOG(err, "Connection[id={}] message handler failed: {}", m_id.id, e.what());
                    stopImpl("message handler failed");
                    co_return;
                }
            }
            catch (std::exception const& e)
            {
                if (!m_closed)
                {
                    LOG(warn, "Connection[id={}] read failed: {}", m_id.id, e.what());
                }

                stopImpl("read failed");
                co_return;
            }
        }
    }

    asio::awaitable<void> writeLoop()
    {
        while (!m_closed && !m_writeQueue.empty())
        {
            auto message = std::move(m_writeQueue.front());
            m_writeQueue.pop_front();

            try
            {
                LOG(debug, "Sending message to connection[id={}]", m_id.id);
                co_await m_channel.writeMessage(m_socket, std::move(message));
            }
            catch (std::exception const& e)
            {
                if (!m_closed)
                {
                    LOG(warn, "Connection[id={}] write failed: {}", m_id.id, e.what());
                }

                m_writing = false;
                stopImpl("write failed");
                co_return;
            }
        }

        m_writing = false;
    }

    void stopImpl(std::string_view reason)
    {
        if (m_closed)
        {
            return;
        }

        m_closed = true;

        m_writeQueue.clear();
        m_writing = false;

        system::error_code ignored;
        m_socket.cancel(ignored);
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        m_socket.close(ignored);

        LOG(info, "Connection[id={}] stopped with {}: {}", m_id.id, m_remoteEndpoint, reason);

        notifyClosed();
    }

    void notifyClosed()
    {
        if (!m_onClosed)
        {
            return;
        }

        try
        {
            m_onClosed();
        }
        catch (std::exception const& e)
        {
            LOG(err, "Connection[id={}] close handler failed: {}", m_id.id, e.what());
        }
    }

    static std::string makeRemoteEndpointString(asio::ip::tcp::socket const& socket)
    {
        system::error_code ec;

        auto const endpoint = socket.remote_endpoint(ec);
        if (ec)
        {
            return "<unknown>";
        }

        auto const address = endpoint.address().to_string();
        if (ec)
        {
            return "<unknown>";
        }

        return address + ":" + std::to_string(endpoint.port());
    }

private:
    asio::ip::tcp::socket m_socket;
    asio::strand<asio::any_io_executor> m_strand;
    MessageChannel m_channel;

    message_handler_t m_onMessage;
    close_handler_t m_onClosed;

    connection_id m_id;
    std::string m_remoteEndpoint;

    std::deque<outgoing_message_t> m_writeQueue;
    std::size_t m_maxQueuedMessages;

    bool m_started{false};
    bool m_closed{false};
    bool m_writing{false};
};

} // namespace s2d::network
