#pragma once

#include "IMessageHandler.hpp"

#include <asio/awaitable.hpp>

namespace s2d::protocol
{
class ClientMessage;
}

namespace s2d::network
{
struct connection_id;

class ServerMessageHandler : public IMessageHandler
{
public:
    asio::awaitable<protocol::ServerMessage> onMessage(connection_id connection_id, protocol::ClientMessage const& message) override;
    void onDisconnect(connection_id connection_id) override;
};
} // namespace s2d::network
