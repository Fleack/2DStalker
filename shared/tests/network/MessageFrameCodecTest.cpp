#include "shared/network/MessageFrameCodec.hpp"

#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

using s2d::network::MessageFrameCodec;

TEST_CASE("MessageFrameCodec converts 32-bit host and network byte order", "[network][framing]")
{
    constexpr std::uint32_t host_value = 0x01020304U;
    auto const network_value = MessageFrameCodec::hostToNetwork32(host_value);

    if constexpr (std::endian::native == std::endian::little)
    {
        REQUIRE(network_value == 0x04030201U);
    }
    else
    {
        REQUIRE(network_value == host_value);
    }

    REQUIRE(MessageFrameCodec::networkToHost32(network_value) == host_value);
}

TEST_CASE("MessageFrameCodec encodes big-endian length prefix", "[network][framing]")
{
    MessageFrameCodec const codec{std::numeric_limits<std::uint32_t>::max()};

    auto const prefix = codec.encodeLengthPrefix(0x01020304U);

    REQUIRE(prefix == MessageFrameCodec::LengthPrefix{0x01U, 0x02U, 0x03U, 0x04U});
    REQUIRE(codec.decodeLengthPrefix(prefix) == 0x01020304U);
}

TEST_CASE("MessageFrameCodec accepts max-size payload frame", "[network][framing]")
{
    constexpr std::uint32_t max_payload_size = 16;
    MessageFrameCodec const codec{max_payload_size};

    auto const prefix = codec.encodeLengthPrefix(max_payload_size);

    REQUIRE(prefix == MessageFrameCodec::LengthPrefix{0x00U, 0x00U, 0x00U, 0x10U});
    REQUIRE(codec.decodeLengthPrefix(prefix) == max_payload_size);
}

TEST_CASE("MessageFrameCodec rejects zero-length payload frame", "[network][framing]")
{
    MessageFrameCodec const codec{16};

    REQUIRE_THROWS_AS(codec.encodeLengthPrefix(0), std::runtime_error);
    REQUIRE_THROWS_AS(codec.decodeLengthPrefix(MessageFrameCodec::LengthPrefix{}), std::runtime_error);
}

TEST_CASE("MessageFrameCodec rejects oversized payload frame", "[network][framing]")
{
    MessageFrameCodec const codec{16};
    constexpr MessageFrameCodec::LengthPrefix oversized_prefix{0x00U, 0x00U, 0x00U, 0x11U};

    REQUIRE_THROWS_AS(codec.encodeLengthPrefix(17), std::runtime_error);
    REQUIRE_THROWS_AS(codec.decodeLengthPrefix(oversized_prefix), std::runtime_error);
}
