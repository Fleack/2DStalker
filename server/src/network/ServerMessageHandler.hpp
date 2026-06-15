#pragma once

#include "shared/network/AMessageHandler.hpp"
#include "shared/protocol/message.pb.h"

#include <asio/awaitable.hpp>

namespace s2d::network
{
struct connection_id;

class ServerMessageHandler
{
public:
    asio::awaitable<protocol::ServerMessage> onMessage(connection_id connection_id, protocol::ClientMessage const& message);
    void onDisconnect(connection_id connection_id) noexcept;
};
} // namespace s2d::network
