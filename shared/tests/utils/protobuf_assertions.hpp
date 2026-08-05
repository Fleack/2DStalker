#pragma once

#include <catch2/catch_test_macros.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>

namespace s2d::test
{

inline void require_messages_equal(
    google::protobuf::Message const& expected,
    google::protobuf::Message const& actual)
{
    REQUIRE(google::protobuf::util::MessageDifferencer::Equals(expected, actual));
}

} // namespace s2d::test
