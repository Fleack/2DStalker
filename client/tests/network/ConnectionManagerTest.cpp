#include "network/Client.hpp"
#include "shared/tests/network/utils/network_fixtures.hpp"
#include "shared/tests/utils/future_assertions.hpp"

#include <chrono>
#include <future>
#include <stdexcept>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch_test_macros.hpp>

using s2d::test::require_ready;
using namespace boost;

TEST_CASE_METHOD(
    s2d::test::network::io_fixture,
    "Connection manager exposes each stable transition",
    "[client][network][connection-manager]")
{
    auto client = network::Client::create(io);
    REQUIRE(client->state() == network::ConnectionState::Disconnected);

    asio::ip::tcp::acceptor acceptor{io, {asio::ip::address_v4::loopback(), 0}};
    auto const endpoint = acceptor.local_endpoint();
    auto connected = spawn(client->connect(endpoint.address(), endpoint.port()));

    for (auto attempt = 0;
         attempt < 4 && client->state() == network::ConnectionState::Disconnected;
         ++attempt)
    {
        REQUIRE(io.poll_one() == 1);
    }
    REQUIRE(client->state() == network::ConnectionState::Connecting);

    restart();
    auto accepted = acceptor.async_accept(asio::use_future);
    REQUIRE(run_until_ready(connected, std::chrono::milliseconds{100}));
    REQUIRE_NOTHROW(connected.get());
    REQUIRE(client->state() == network::ConnectionState::Connected);
    require_ready(accepted);
    auto serverSocket = accepted.get();

    restart();
    auto disconnected = spawn(client->disconnect());
    for (auto attempt = 0;
         attempt < 4 && client->state() == network::ConnectionState::Connected;
         ++attempt)
    {
        REQUIRE(io.poll_one() == 1);
    }
    REQUIRE(client->state() == network::ConnectionState::Disconnecting);

    restart();
    run();
    REQUIRE_NOTHROW(disconnected.get());
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
    (void)serverSocket;
}

TEST_CASE_METHOD(
    s2d::test::network::io_fixture,
    "Connection manager rejects events invalid for current state",
    "[client][network][connection-manager]")
{
    auto client = network::Client::create(io);

    auto disconnected = spawn(client->disconnect());
    run();
    REQUIRE_THROWS_AS(disconnected.get(), std::logic_error);
    REQUIRE(client->state() == network::ConnectionState::Disconnected);

    restart();
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::address_v4::loopback(), 0}};
    auto const endpoint = acceptor.local_endpoint();
    auto accepted = acceptor.async_accept(asio::use_future);
    auto connected = spawn(client->connect(endpoint.address(), endpoint.port()));
    auto duplicate = spawn(client->connect(endpoint.address(), endpoint.port()));
    REQUIRE(run_until_ready(connected, std::chrono::milliseconds{100}));
    REQUIRE(run_until_ready(duplicate, std::chrono::milliseconds{100}));

    REQUIRE_NOTHROW(connected.get());
    REQUIRE_THROWS_AS(duplicate.get(), std::logic_error);
    REQUIRE(client->state() == network::ConnectionState::Connected);
    require_ready(accepted);
}

TEST_CASE_METHOD(
    s2d::test::network::io_fixture,
    "Disconnect cancels connect before io context starts and permits retry",
    "[client][network][connection-manager][cancellation]")
{
    auto client = network::Client::create(io);

    asio::ip::tcp::acceptor firstAcceptor{io, {asio::ip::address_v4::loopback(), 0}};
    auto const firstEndpoint = firstAcceptor.local_endpoint();
    auto cancelledConnect = spawn(
        client->connect(firstEndpoint.address(), firstEndpoint.port()));
    auto cancelledDisconnect = spawn(client->disconnect());

    REQUIRE(client->state() == network::ConnectionState::Disconnected);
    run();

    REQUIRE_THROWS_AS(cancelledConnect.get(), std::system_error);
    REQUIRE_NOTHROW(cancelledDisconnect.get());
    REQUIRE(client->state() == network::ConnectionState::Disconnected);

    restart();
    asio::ip::tcp::acceptor secondAcceptor{io, {asio::ip::address_v4::loopback(), 0}};
    auto const secondEndpoint = secondAcceptor.local_endpoint();
    auto accepted = secondAcceptor.async_accept(asio::use_future);
    auto retried = spawn(client->connect(secondEndpoint.address(), secondEndpoint.port()));
    REQUIRE(run_until_ready(retried, std::chrono::milliseconds{100}));

    REQUIRE_NOTHROW(retried.get());
    require_ready(accepted);
    REQUIRE(client->state() == network::ConnectionState::Connected);
}

TEST_CASE_METHOD(
    s2d::test::network::io_fixture,
    "Failed connect completes cancellation and returns to disconnected",
    "[client][network][connection-manager][cancellation]")
{
    auto client = network::Client::create(io);
    auto connecting = spawn(client->connect(asio::ip::address_v4::loopback(), 0));
    auto disconnecting = spawn(client->disconnect());

    run();

    REQUIRE_THROWS_AS(connecting.get(), std::system_error);
    REQUIRE_NOTHROW(disconnecting.get());
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
}
