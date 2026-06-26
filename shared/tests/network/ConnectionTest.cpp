#include "shared/network/Connection.hpp"
#include "shared/tests/utils/protobuf_assertions.hpp"
#include "utils/message_io.hpp"
#include "utils/network_fixtures.hpp"
#include "utils/network_test_constants.hpp"
#include "utils/protocol_messages.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#include <asio/awaitable.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
using connection_t = s2d::network::Connection<
    s2d::protocol::ClientMessage,
    s2d::protocol::ServerMessage>;
} // namespace

using s2d::test::require_messages_equal;
using s2d::test::network::available_bytes_after_wait;
using s2d::test::network::close_socket;
using s2d::test::network::make_ping_request;
using s2d::test::network::make_pong_response;
using s2d::test::network::max_message_bytes;
using s2d::test::network::read_message;
using s2d::test::network::write_message;

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "Connection dispatches incoming message to explicit handler",
    "[network][connection]")
{
    std::optional<std::uint64_t> received_request_id;
    bool closed{false};

    auto connection = connection_t::create(
        s2d::network::connection_id{1},
        std::move(sockets.server),
        connection_t::Config{max_message_bytes},
        [&received_request_id](s2d::network::connection_id, s2d::protocol::ClientMessage message) -> asio::awaitable<void> {
            received_request_id = message.request_id();
            co_return;
        },
        [&closed](s2d::network::connection_id) {
            closed = true;
        });
    connection->start();

    auto message = make_ping_request(7);
    auto available_bytes = spawn([&]() -> asio::awaitable<std::size_t> {
        co_await write_message(sockets.client, message);
        auto const bytes = co_await available_bytes_after_wait(sockets.client);
        close_socket(sockets.client);
        co_return bytes;
    });

    run();

    REQUIRE(available_bytes.get() == 0);
    REQUIRE(received_request_id == message.request_id());
    REQUIRE(closed);
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "Connection rejects empty message handler",
    "[network][connection]")
{
    REQUIRE_THROWS_AS(
        connection_t::create(
            s2d::network::connection_id{1},
            std::move(sockets.server),
            connection_t::Config{max_message_bytes},
            {}),
        std::invalid_argument);
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "Connection send writes outgoing message",
    "[network][connection]")
{
    auto connection = connection_t::create(
        s2d::network::connection_id{1},
        std::move(sockets.server),
        connection_t::Config{max_message_bytes},
        [](s2d::network::connection_id, s2d::protocol::ClientMessage) -> asio::awaitable<void> {
            co_return;
        });

    auto expected_message = make_pong_response(11);
    connection->send(expected_message);

    auto read = spawn([&]() -> asio::awaitable<s2d::protocol::ServerMessage> {
        auto message = co_await read_message<s2d::protocol::ServerMessage>(sockets.client);
        close_socket(sockets.client);
        co_return message;
    });

    run();

    auto received_message = read.get();
    require_messages_equal(expected_message, received_message);
}
