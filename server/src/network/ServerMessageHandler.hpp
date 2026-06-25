#pragma once

#include "shared/protocol/message.pb.h"

#include <asio/awaitable.hpp>

namespace s2d::game
{
class WorldService;
}

namespace s2d::network
{
struct connection_id;

class ServerMessageHandler
{
public:
    explicit ServerMessageHandler(game::WorldService& worldService) noexcept;

    asio::awaitable<protocol::ServerMessage> onMessage(connection_id connection_id, protocol::ClientMessage const& message);
    void onDisconnect(connection_id connection_id) noexcept;

private:
    game::WorldService& m_worldService;
};
} // namespace s2d::network
