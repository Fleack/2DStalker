#pragma once

#include "shared/protocol/message.pb.h"

namespace s2d::test::network::utils
{
using namespace s2d::network;

class MockMessageHandler final
{
public:
    asio::awaitable<protocol::ServerMessage> onMessage(connection_id, protocol::ClientMessage const&)
    {
        co_return response;
    }

    void onDisconnect(connection_id)
    {
    }

    protocol::ServerMessage response;
};

using mock_server_message_types_t = MessageTypes<protocol::ClientMessage, protocol::ServerMessage>;
using mock_client_message_types_t = MessageTypes<protocol::ServerMessage, protocol::ClientMessage>;
using mock_side_t = NetworkSide<mock_server_message_types_t, MockMessageHandler>;
using mock_connection_t = Connection<mock_side_t>;
} // namespace s2d::test::network::utils
