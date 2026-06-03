#pragma once

#include "connection_id.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/protocol/message.pb.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>

namespace s2d::network
{

class Connection;

class ConnectionWriter
{
public:
    ConnectionWriter(
        asio::ip::tcp::socket& socket,
        connection_id id,
        std::uint32_t max_message_bytes,
        std::function<void()> onError);

    void send(protocol::ServerMessage message, std::shared_ptr<Connection> connection);

private:
    void enqueue(protocol::ServerMessage message, std::shared_ptr<Connection> connection);
    asio::awaitable<void> drain(std::shared_ptr<Connection> connection);

private:
    std::deque<protocol::ServerMessage> m_queue;
    asio::strand<asio::any_io_executor> m_strand;
    MessageChannel m_messageChannel;
    std::function<void()> m_onError;
    asio::ip::tcp::socket& m_socket;
    connection_id m_id;
    bool m_writeInProgress{false};
};

} // namespace s2d::network
