#pragma once

namespace s2d::network
{
template <typename IncomingMessage, typename OutcomingMessage>
struct MessageTypes
{
    using incoming_message_t = IncomingMessage;
    using outcoming_message_t = OutcomingMessage;
};

template <typename MessageTypes>
concept AMessageTypes = requires {
    typename MessageTypes::incoming_message_t;
    typename MessageTypes::outcoming_message_t;
};
} // namespace s2d::network
