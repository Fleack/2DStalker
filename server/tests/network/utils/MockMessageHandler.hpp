#pragma once

namespace s2d::test::network::utils
{
using namespace s2d::network;
using namespace s2d::protocol;

class MockMessageHandler final : public IMessageHandler
{
public:
    asio::awaitable<ServerMessage> onMessage(connection_id, ClientMessage const&) override
    {
        co_return response;
    }

    void onDisconnect(connection_id) override
    {
    }

    ServerMessage response;
};
} // namespace s2d::test::network::utils
