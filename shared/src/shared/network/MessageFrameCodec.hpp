#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace s2d::network
{

class MessageFrameCodec
{
public:
    static constexpr std::size_t length_prefix_size = sizeof(std::uint32_t);
    using LengthPrefix = std::array<std::uint8_t, length_prefix_size>;

    explicit constexpr MessageFrameCodec(std::uint32_t max_payload_size) noexcept;

    static std::uint32_t hostToNetwork32(std::uint32_t value) noexcept;
    static std::uint32_t networkToHost32(std::uint32_t value) noexcept;

    [[nodiscard]] LengthPrefix encodeLengthPrefix(std::uint32_t payload_size) const;
    [[nodiscard]] std::uint32_t decodeLengthPrefix(LengthPrefix prefix) const;

    [[nodiscard]] constexpr std::uint32_t maxPayloadSize() const noexcept;

private:
    void validatePayloadSize(std::uint32_t payload_size) const;
    static LengthPrefix encodeUncheckedLengthPrefix(std::uint32_t payload_size) noexcept;

private:
    std::uint32_t m_max_payload_size;
};

inline constexpr MessageFrameCodec::MessageFrameCodec(std::uint32_t max_payload_size) noexcept
    : m_max_payload_size(max_payload_size)
{
}

inline std::uint32_t MessageFrameCodec::hostToNetwork32(std::uint32_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
    {
        return std::byteswap(value);
    }

    return value;
}

inline std::uint32_t MessageFrameCodec::networkToHost32(std::uint32_t value) noexcept
{
    return hostToNetwork32(value);
}

inline MessageFrameCodec::LengthPrefix MessageFrameCodec::encodeLengthPrefix(std::uint32_t payload_size) const
{
    validatePayloadSize(payload_size);
    return encodeUncheckedLengthPrefix(payload_size);
}

inline std::uint32_t MessageFrameCodec::decodeLengthPrefix(LengthPrefix prefix) const
{
    auto const network_size = std::bit_cast<std::uint32_t>(prefix);
    auto const payload_size = networkToHost32(network_size);
    validatePayloadSize(payload_size);
    return payload_size;
}

inline constexpr std::uint32_t MessageFrameCodec::maxPayloadSize() const noexcept
{
    return m_max_payload_size;
}

inline void MessageFrameCodec::validatePayloadSize(std::uint32_t payload_size) const
{
    if (payload_size == 0)
    {
        throw std::runtime_error("Message frame has zero-length protobuf payload");
    }

    if (payload_size > m_max_payload_size)
    {
        throw std::runtime_error("Message frame is bigger than max_message_size");
    }
}

inline MessageFrameCodec::LengthPrefix MessageFrameCodec::encodeUncheckedLengthPrefix(
    std::uint32_t payload_size) noexcept
{
    auto const network_size = hostToNetwork32(payload_size);
    return std::bit_cast<LengthPrefix>(network_size);
}

} // namespace s2d::network
