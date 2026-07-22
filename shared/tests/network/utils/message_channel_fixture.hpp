#pragma once

#include "network_fixtures.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/tests/utils/protobuf_assertions.hpp"

#include <utility>

#include <boost/asio/awaitable.hpp>
#include <catch2/catch_test_macros.hpp>

namespace s2d::test::network
{

struct message_channel_fixture : socket_pair_fixture
{
    s2d::network::MessageChannel channel;

    template <typename Message>
    void expect_message_delivered(Message message)
    {
        auto test = spawn(deliver_message(std::move(message)));

        run();

        REQUIRE_NOTHROW(test.get());
    }

private:
    template <typename Message>
    asio::awaitable<void> deliver_message(Message message)
    {
        co_await channel.writeMessage(sockets.client, message);

        auto received_message = co_await channel.readMessage<Message>(sockets.server);
        s2d::test::require_messages_equal(message, received_message);
    }
};

} // namespace s2d::test::network
