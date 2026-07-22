#include "shared/network/MessageFrameCodec.hpp"
#include "shared/protocol/message.pb.h"
#include "utils/message_channel_fixture.hpp"
#include "utils/protocol_messages.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
using namespace boost;

asio::awaitable<void> write_raw_frame(
    asio::ip::tcp::socket& socket,
    s2d::network::MessageFrameCodec::LengthPrefix prefix,
    std::vector<std::uint8_t> payload = {})
{
    co_await asio::async_write(socket, asio::buffer(prefix), asio::use_awaitable);
    if (!payload.empty())
    {
        co_await asio::async_write(
            socket,
            asio::buffer(payload.data(), payload.size()),
            asio::use_awaitable);
    }
}
} // namespace

TEST_CASE_METHOD(
    s2d::test::network::message_channel_fixture,
    "MessageChannel writes and reads ClientMessage")
{
    s2d::protocol::ClientMessage message;
    message.set_request_id(1);

    SECTION("ping")
    {
        message.mutable_ping()->set_timestamp(123456789);

        expect_message_delivered(message);
    }

    SECTION("state_snapshot")
    {
        message.mutable_state_snapshot();

        expect_message_delivered(message);
    }
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "MessageChannel rejects zero-length frame")
{
    s2d::network::MessageChannel channel;

    auto read = spawn([&]() -> asio::awaitable<void> {
        co_await write_raw_frame(sockets.client, s2d::network::MessageFrameCodec::LengthPrefix{});
        (void)co_await channel.readMessage<s2d::protocol::ClientMessage>(sockets.server);
    });

    run();

    REQUIRE_THROWS_AS(read.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "MessageChannel rejects invalid protobuf payload")
{
    s2d::network::MessageChannel channel;
    s2d::network::MessageFrameCodec const codec{1024};

    auto read = spawn([&]() -> asio::awaitable<void> {
        co_await write_raw_frame(
            sockets.client,
            codec.encodeLengthPrefix(1),
            std::vector<std::uint8_t>{0xFFU});
        (void)co_await channel.readMessage<s2d::protocol::ClientMessage>(sockets.server);
    });

    run();

    REQUIRE_THROWS_AS(read.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::network::message_channel_fixture,
    "MessageChannel writes and reads ServerMessage")
{
    s2d::protocol::ServerMessage message;
    message.set_request_id(2);

    SECTION("pong")
    {
        message.set_status(s2d::protocol::STATUS_OK);
        message.mutable_pong()->set_timestamp(987654321);

        expect_message_delivered(message);
    }

    SECTION("state_snapshot")
    {
        message.set_status(s2d::protocol::STATUS_OK);
        message.mutable_state_snapshot()->set_state_json(R"({"x":10,"y":20})");

        expect_message_delivered(message);
    }

    SECTION("error")
    {
        message.set_status(s2d::protocol::STATUS_ERROR);
        message.mutable_error()->set_code(500);
        message.mutable_error()->set_message("Internal server error");

        expect_message_delivered(message);
    }
}
