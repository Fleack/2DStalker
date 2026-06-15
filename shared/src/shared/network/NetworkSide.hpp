#pragma once

#include "shared/network/MessageTypes.hpp"

namespace s2d::network
{
template <AMessageTypes MessageTypes, typename Handler>
struct NetworkSide
{
    using incoming_message_t = MessageTypes::incoming_message_t;
    using outcoming_message_t = MessageTypes::outcoming_message_t;
    using handler_t = Handler;
};

template <typename NetworkSide>
concept ANetworkSide = requires {
    typename NetworkSide::incoming_message_t;
    typename NetworkSide::outcoming_message_t;
    typename NetworkSide::handler_t;
};
} // namespace s2d::network
