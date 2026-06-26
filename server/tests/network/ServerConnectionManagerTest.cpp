#include "server/src/game/WorldService.hpp"
#include "server/src/network/ServerConnectionManager.hpp"
#include "server/src/network/ServerMessageHandler.hpp"
#include "shared/tests/network/utils/message_io.hpp"
#include "shared/tests/network/utils/network_fixtures.hpp"
#include "shared/tests/network/utils/network_test_constants.hpp"
#include "shared/tests/network/utils/protocol_messages.hpp"

#include <asio/awaitable.hpp>
#include <catch2/catch_test_macros.hpp>

using s2d::test::network::close_socket;
using s2d::test::network::make_ping_request;
using s2d::test::network::max_message_bytes;
using s2d::test::network::read_message;
using s2d::test::network::write_message;

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "ServerConnectionManager wires shared Connection to server handler",
    "[server][network]")
{
    s2d::game::WorldService world_service;
    s2d::network::ServerMessageHandler handler{world_service};
    s2d::network::ServerConnectionManager manager;
    auto connection = manager.create(
        std::move(sockets.server),
        handler,
        max_message_bytes);
    REQUIRE(connection);
    auto const connection_id = connection->getId();
    connection->start();

    auto request = make_ping_request(42);
    auto response_future = spawn([&]() -> asio::awaitable<s2d::protocol::ServerMessage> {
        co_await write_message(sockets.client, request);
        auto response = co_await read_message<s2d::protocol::ServerMessage>(sockets.client);
        close_socket(sockets.client);
        co_return response;
    }());

    run();

    auto response = response_future.get();
    REQUIRE(response.request_id() == request.request_id());
    REQUIRE(response.status() == s2d::protocol::STATUS_OK);
    REQUIRE(response.pong().timestamp() == request.ping().timestamp());
    REQUIRE_FALSE(manager.get(connection_id));
}
