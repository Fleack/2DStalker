#include "Server.hpp"

namespace s2d::network
{

Server::Server(asio::io_context& io_context, uint16_t port) noexcept : port_{port}, io_context_{io_context} {}

asio::awaitable<void> Server::start() noexcept
{
    co_return;
}

} // namespace s2d::network
