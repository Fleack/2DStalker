#pragma once

#include "server/src/network/ServerMessageHandler.hpp"
#include "shared/network/Connection.hpp"
#include "shared/protocol/message.pb.h"

namespace s2d::network
{
using server_message_types_t = MessageTypes<protocol::ClientMessage, protocol::ServerMessage>;
using server_side_t = NetworkSide<server_message_types_t, ServerMessageHandler>;
using server_connection_t = Connection<server_side_t>;

static_assert(AMessageHandler<ServerMessageHandler, server_message_types_t>);
} // namespace s2d::network
