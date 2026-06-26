#include "game/WorldService.hpp"
#include "network/ServerMessageHandler.hpp"
#include "network/TcpServer.hpp"
#include "network/tcp_server_config.hpp"
#include "shared/logger/logger.hpp"
#include "shared/network/Connection.hpp"

#include <asio.hpp>
#include <cstdint>

#include <spdlog/spdlog.h>

using asio::ip::tcp;

namespace
{

asio::awaitable<void> startServer(s2d::network::TcpServer& server)
{
    co_await server.start();
}

} // namespace

int main()
{
    try
    {
        asio::io_context io;
        s2d::network::tcp_server_config cfg;
        s2d::game::WorldService worldService;
        s2d::network::ServerMessageHandler handler{worldService};
        s2d::network::TcpServer server{io, cfg, handler};

        asio::co_spawn(io, startServer(server), asio::detached);

        io.run();
    }
    catch (std::exception& e)
    {
        LOG(err, "Exception: {}", e.what());
    }
}
