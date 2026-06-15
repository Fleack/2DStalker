#pragma once

#include "shared/network/MessageTypes.hpp"
#include "shared/network/connection_id.hpp"

#include <asio/awaitable.hpp>

namespace s2d::network
{

template <typename Handler, typename Types>
concept AMessageHandler =
    AMessageTypes<Types> &&
    requires(
        Handler& handler,
        connection_id id,
        typename Types::incoming_message_t message) {
        {
            handler.onMessage(id, std::move(message))
        } -> std::same_as<asio::awaitable<typename Types::outcoming_message_t>>;

        {
            handler.onDisconnect(id)
        } noexcept -> std::same_as<void>;
    };
} // namespace s2d::network
