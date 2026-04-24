#pragma once

#include <cstdint>

namespace s2d::network
{
struct network_config
{
    std::uint16_t port = 1234;
    std::uint32_t max_message_bytes = 1024 * 1024;
};
} // namespace s2d::network
