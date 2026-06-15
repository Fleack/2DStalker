#include "shared/network/MessageChannel.hpp"
#include "shared/tests/network/utils/ConnectedSocketPair.hpp"

#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/use_future.hpp>
#include <catch2/catch_test_macros.hpp>
#include <google/protobuf/util/message_differencer.h>

namespace
{

template <class MessageType>
void checkWriteRead(MessageType const& sentMessage)
{
    asio::io_context io;

    auto [clientSocket, serverSocket] = makeConnectedSocketPair(io);

    s2d::network::MessageChannel channel;

    auto testFuture = asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            co_await channel.writeMessage(clientSocket, sentMessage);

            auto receivedMessage =
                co_await channel.readMessage<MessageType>(serverSocket);

            REQUIRE(google::protobuf::util::MessageDifferencer::Equals(sentMessage, receivedMessage));

            co_return;
        },
        asio::use_future);

    io.run();

    REQUIRE_NOTHROW(testFuture.get());
}

std::uint64_t requestId{1};

} // namespace

TEST_CASE("MessageChannel writes and reads ClientMessage")
{
    s2d::protocol::ClientMessage message;

    message.set_request_id(requestId++);
    SECTION("ping")
    {
        message.mutable_ping()->set_timestamp(123456789);

        checkWriteRead(message);
    }

    SECTION("state_snapshot")
    {
        message.mutable_state_snapshot();

        checkWriteRead(message);
    }
}

TEST_CASE("MessageChannel writes and reads ServerMessage")
{
    s2d::protocol::ServerMessage message;
    message.set_request_id(requestId++);

    SECTION("pong")
    {
        message.set_status(s2d::protocol::STATUS_OK);
        message.mutable_pong()->set_timestamp(987654321);

        checkWriteRead(message);
    }

    SECTION("state_snapshot")
    {
        message.set_status(s2d::protocol::STATUS_OK);
        message.mutable_state_snapshot()->set_state_json(R"({"x":10,"y":20})");

        checkWriteRead(message);
    }

    SECTION("error")
    {
        message.set_status(s2d::protocol::STATUS_ERROR);
        message.mutable_error()->set_code(500);
        message.mutable_error()->set_message("Internal server error");

        checkWriteRead(message);
    }
}
