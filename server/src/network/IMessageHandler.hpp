#pragma once

#include "connection_id.hpp"
#include "shared/protocol/message.pb.h"
#include <asio/awaitable.hpp>

namespace s2d::network
{
class IMessageHandler
{
public:
    virtual ~IMessageHandler() = default;

    virtual asio::awaitable<protocol::ServerMessage> onMessage(connection_id connection_id, protocol::ClientMessage const& message) = 0;
    virtual void onDisconnect(connection_id connection_id) = 0;
};
} // namespace s2d::network
