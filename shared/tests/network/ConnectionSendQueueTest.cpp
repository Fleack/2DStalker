#include "shared/network/Connection.hpp"
#include "utils/message_io.hpp"
#include "utils/network_fixtures.hpp"
#include "utils/network_test_constants.hpp"
#include "utils/protocol_messages.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/post.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
using connection_t = s2d::network::Connection<
    s2d::protocol::ClientMessage,
    s2d::protocol::ServerMessage>;
} // namespace

using s2d::test::network::make_pong_response;
using s2d::test::network::max_message_bytes;
using s2d::test::network::read_message;

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "Connection serializes enqueued sends through one write queue",
    "[network][connection]")
{
    constexpr std::size_t message_count = 128;

    auto connection = connection_t::create(
        s2d::network::connection_id{1},
        std::move(sockets.server),
        connection_t::Config{.maxMessageBytes = max_message_bytes},
        [](s2d::network::connection_id, s2d::protocol::ClientMessage) -> asio::awaitable<void> {
            co_return;
        });

    for (std::size_t i = 0; i < message_count; ++i)
    {
        auto const request_id = i + 1;
        asio::post(
            io,
            [connection, message = make_pong_response(request_id, request_id)]() mutable {
                connection->send(std::move(message));
            });
    }

    auto read = spawn([&]() -> asio::awaitable<std::vector<std::uint64_t>> {
        std::vector<std::uint64_t> request_ids;
        request_ids.reserve(message_count);

        for (std::size_t i = 0; i < message_count; ++i)
        {
            auto message = co_await read_message<s2d::protocol::ServerMessage>(sockets.client);
            request_ids.push_back(message.request_id());
        }

        co_return request_ids;
    }());

    run();

    auto request_ids = read.get();
    REQUIRE(request_ids.size() == message_count);

    std::ranges::sort(request_ids);
    for (std::size_t i = 0; i < message_count; ++i)
    {
        REQUIRE(request_ids[i] == i + 1);
    }
}
