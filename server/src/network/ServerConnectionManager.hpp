#pragma once

#include "server/src/network/ServerMessageHandler.hpp"
#include "shared/network/Connection.hpp"
#include "shared/network/connection_id.hpp"
#include "shared/protocol/message.pb.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace s2d::network
{

using server_connection_t = Connection<protocol::ClientMessage, protocol::ServerMessage>;

class ServerConnectionManager
{
public:
    ServerConnectionManager() = default;

    std::shared_ptr<server_connection_t> create(asio::ip::tcp::socket&& socket, ServerMessageHandler& handler, std::uint32_t max_message_bytes);
    bool remove(connection_id id);
    std::shared_ptr<server_connection_t> get(connection_id id) const;

    void send(connection_id id, protocol::ServerMessage const& message) const;
    void broadcast(protocol::ServerMessage const& message) const;

    void stopAll() noexcept;

private:
    asio::awaitable<void> handleMessage(ServerMessageHandler& handler, connection_id id, protocol::ClientMessage message) const;
    void handleDisconnect(ServerMessageHandler& handler, connection_id id);

    std::vector<std::shared_ptr<server_connection_t>> makeSnapshot() const;

private:
    std::unordered_map<connection_id, std::shared_ptr<server_connection_t>> m_connections;
    bool m_stopped{false};
    connection_id m_nextConnectionId{0};
};

} // namespace s2d::network
