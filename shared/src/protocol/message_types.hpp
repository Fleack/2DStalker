#pragma once

#include <string_view>

namespace protocol::message_types
{
inline constexpr std::string_view connect = "connect";
inline constexpr std::string_view submit_turn = "submit_turn";
inline constexpr std::string_view state_snapshot = "state_snapshot";
} // namespace protocol::message_types
