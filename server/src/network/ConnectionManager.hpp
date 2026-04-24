#pragma once

#include "Connection.hpp"
#include "connection_id.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace s2d::network
{

class ConnectionManager
{
public:
    ConnectionManager() = default;

    std::shared_ptr<Connection> create(asio::ip::tcp::socket&& socket, IMessageHandler& handler, std::uint32_t max_message_bytes);
    bool remove(connection_id id);
    std::shared_ptr<Connection> get(connection_id id) const;

    asio::awaitable<void> send(connection_id id, protocol::ServerMessage const& message) const;
    asio::awaitable<void> broadcast(protocol::ServerMessage const& message) const;

    void stopAll() noexcept;

private:
    std::vector<std::shared_ptr<Connection>> makeSnapshot() const;

private:
    std::unordered_map<connection_id, std::shared_ptr<Connection>> m_connections;
    bool m_stopped{false};
    connection_id m_nextConnectionId{0};
};

} // namespace s2d::network
