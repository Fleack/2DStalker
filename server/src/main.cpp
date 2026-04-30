#include "shared/logger/logger.hpp"
#include "network/Connection.hpp"
#include "network/ServerMessageHandler.hpp"
#include "network/TcpServer.hpp"
#include "network/network_config.hpp"

#include <asio.hpp>
#include <cstdint>

#include <spdlog/spdlog.h>

using asio::ip::tcp;

int main()
{
    try
    {
        asio::io_context io;
        s2d::network::network_config cfg;
        s2d::network::ServerMessageHandler handler;
        s2d::network::TcpServer server{io, cfg, handler};

        asio::co_spawn(io, [&server]() -> asio::awaitable<void> { co_await server.start(); }, asio::detached);

        io.run();
    }
    catch (std::exception& e)
    {
        LOG(err, "Fatal: {}", e.what());
    }
}
