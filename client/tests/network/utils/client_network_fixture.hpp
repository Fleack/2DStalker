#pragma once

#include "network/Client.hpp"
#include "shared/tests/network/utils/network_fixtures.hpp"
#include "shared/tests/utils/future_assertions.hpp"

#include <chrono>
#include <future>
#include <memory>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch_test_macros.hpp>

namespace s2d::test::client::network
{

using namespace boost;

struct client_network_fixture : s2d::test::network::io_fixture
{
    std::shared_ptr<::network::Client> client{::network::Client::create(io)};

    asio::ip::tcp::socket connect_to_server()
    {
        asio::ip::tcp::acceptor acceptor{io, {asio::ip::address_v4::loopback(), 0}};
        auto const endpoint = acceptor.local_endpoint();

        auto accepted = acceptor.async_accept(asio::use_future);
        auto connected = asio::co_spawn(
            io,
            client->connect(endpoint.address(), endpoint.port()),
            asio::use_future);

        for (auto attempt = 0;
             attempt < 100 &&
             (accepted.wait_for(std::chrono::seconds{0}) != std::future_status::ready || connected.wait_for(std::chrono::seconds{0}) != std::future_status::ready);
             ++attempt)
        {
            io.run_for(std::chrono::milliseconds{10});
        }

        s2d::test::require_ready(accepted);
        s2d::test::require_ready(connected);
        REQUIRE_NOTHROW(connected.get());

        auto server_socket = accepted.get();
        io.restart();

        return server_socket;
    }
};

} // namespace s2d::test::client::network
