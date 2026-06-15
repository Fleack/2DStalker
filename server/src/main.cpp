#include "network/ServerMessageHandler.hpp"
#include "network/TcpServer.hpp"
#include "network/tcp_server_config.hpp"
#include "shared/logger/logger.hpp"
#include "shared/src/shared/network/Connection.hpp"

#include <asio.hpp>
#include <cstdint>

#include <spdlog/spdlog.h>

using asio::ip::tcp;

int main()
{
    try
    {
        asio::io_context io;
        s2d::network::tcp_server_config cfg;
        s2d::network::ServerMessageHandler handler;
        s2d::network::TcpServer server{io, cfg, handler};

        asio::co_spawn(io, [&server]() -> asio::awaitable<void> { co_await server.start(); }, asio::detached);

        io.run();
    }
    catch (std::exception& e)
    {
        LOG(err, "Exception: {}", e.what());
    }
}
