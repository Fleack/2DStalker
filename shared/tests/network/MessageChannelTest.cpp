#include "shared/protocol/message.pb.h"
#include "utils/message_channel_fixture.hpp"

#include <catch2/catch_test_macros.hpp>

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
