#pragma once

#include <asio/use_future.hpp>

struct ConnectedSocketPair
{
    asio::ip::tcp::socket client;
    asio::ip::tcp::socket server;
};

inline ConnectedSocketPair makeConnectedSocketPair(asio::io_context& io)
{
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
    asio::ip::tcp::endpoint const endpoint{
        asio::ip::address_v4::loopback(),
        acceptor.local_endpoint().port(),
    };

    auto accepted = acceptor.async_accept(asio::use_future);

    asio::ip::tcp::socket clientSocket{io};
    auto connected = clientSocket.async_connect(endpoint, asio::use_future);

    io.run();
    connected.get();
    auto serverSocket = accepted.get();
    io.restart();

    return {std::move(clientSocket), std::move(serverSocket)};
}
