#pragma once

#include <cstdint>

#include <asio/awaitable.hpp>

namespace asio
{
class io_context;
}

namespace s2d::network
{
class Server
{
public:
    Server(asio::io_context& io_context, uint16_t port) noexcept;

    asio::awaitable<void> start() noexcept;

private:
    std::uint16_t port_;

    asio::io_context& io_context_;
};
} // namespace s2d::network
