#include "server/src/network/Connection.hpp"
#include "server/src/network/IMessageHandler.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/protocol/message.pb.h"
#include "utils/ConnectedSocketPair.hpp"
#include "utils/MockMessageHandler.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <utility>
#include <vector>

#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
constexpr std::uint32_t maxMessageBytes = 1024 * 1024;

asio::awaitable<std::vector<std::uint64_t>> readServerMessageRequestIds(
    asio::ip::tcp::socket& socket,
    std::size_t count)
{
    s2d::network::MessageChannel channel{maxMessageBytes};
    std::vector<std::uint64_t> requestIds;
    requestIds.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        auto message = co_await channel.readMessage<s2d::protocol::ServerMessage>(socket);
        requestIds.push_back(message.request_id());
    }

    co_return requestIds;
}
} // namespace

using namespace s2d::test;

TEST_CASE("Connection serializes enqueued sends through one write queue", "[network][connection]")
{
    constexpr std::size_t messageCount = 128;

    asio::io_context io;
    auto [client, server] = makeConnectedSocketPair(io);

    network::utils::MockMessageHandler handler;
    s2d::protocol::ServerMessage response;
    response.set_status(s2d::protocol::STATUS_OK);
    handler.response = std::move(response);

    auto connection = std::make_shared<s2d::network::Connection>(
        s2d::network::connection_id{1},
        std::move(server),
        handler,
        maxMessageBytes,
        nullptr);

    auto makeServerMessage = []() {
        static uint64_t requestId{0};
        ++requestId;

        s2d::protocol::ServerMessage message;
        message.set_request_id(requestId);
        message.set_status(s2d::protocol::STATUS_OK);
        message.mutable_pong()->set_timestamp(requestId);
        return message;
    };

    for (std::size_t i = 0; i < messageCount; ++i)
    {
        asio::post(
            io,
            [connection, message = makeServerMessage()]() mutable {
                connection->send(std::move(message));
            });
    }

    auto readResult = asio::co_spawn(
        io,
        readServerMessageRequestIds(client, messageCount),
        asio::use_future);

    io.run();

    auto requestIds = readResult.get();
    REQUIRE(requestIds.size() == messageCount);

    std::ranges::sort(requestIds);
    for (std::size_t i = 0; i < messageCount; ++i)
    {
        REQUIRE(requestIds[i] == i + 1);
    }
}
