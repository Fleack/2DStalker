#include "network/Client.hpp"
#include "shared/tests/network/utils/message_io.hpp"
#include "shared/tests/network/utils/protocol_messages.hpp"
#include "utils/client_network_fixture.hpp"

#include <chrono>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace boost;

using s2d::test::network::close_socket;
using s2d::test::network::make_pong_response;
using s2d::test::network::read_message;
using s2d::test::network::write_message;

namespace
{
s2d::protocol::PingRequest makePing(std::uint64_t timestamp)
{
    s2d::protocol::PingRequest request;
    request.set_timestamp(timestamp);
    return request;
}
} // namespace

TEST_CASE("Client validates shared network config", "[client][network]")
{
    asio::io_context io;

    REQUIRE_THROWS_AS(
        network::Client::create(
            io,
            network::Config{
                .requestTimeout = std::chrono::steady_clock::duration::zero(),
            }),
        std::invalid_argument);
    REQUIRE_THROWS_AS(
        network::Client::create(
            io,
            network::Config{
                .maxPendingRequests = 0,
            }),
        std::invalid_argument);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client rejects request while disconnected",
    "[client][network]")
{
    auto sent = spawn(client->sendRequest(makePing(1)));

    run();

    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client failed connect returns to disconnected and can retry",
    "[client][network][connection-manager]")
{
    asio::ip::tcp::acceptor unavailableEndpoint{io};
    unavailableEndpoint.open(asio::ip::tcp::v4());
    unavailableEndpoint.bind({asio::ip::address_v4::loopback(), 0});
    auto const endpoint = unavailableEndpoint.local_endpoint();

    auto failed = spawn(client->connect(endpoint.address(), endpoint.port()));

    run();
    REQUIRE_THROWS_AS(failed.get(), std::system_error);
    REQUIRE(client->state() == network::ConnectionState::Disconnected);

    restart();
    auto serverSocket = connect_to_server();
    REQUIRE(client->state() == network::ConnectionState::Connected);

    close_socket(serverSocket);
    run();
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client rejects duplicate connect and joins duplicate disconnect",
    "[client][network][connection-manager]")
{
    auto serverSocket = connect_to_server();

    auto duplicateConnect = spawn(
        client->connect(asio::ip::address_v4::loopback(), 1));
    REQUIRE(run_until_ready(duplicateConnect, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(duplicateConnect.get(), std::logic_error);

    auto firstDisconnect = spawn(client->disconnect());
    auto duplicateDisconnect = spawn(client->disconnect());
    run();

    REQUIRE_NOTHROW(firstDisconnect.get());
    REQUIRE_NOTHROW(duplicateDisconnect.get());
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
    (void)serverSocket;
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client disconnect completion can be reused across connection cycles",
    "[client][network][connection-manager]")
{
    for (auto cycle = 0; cycle < 2; ++cycle)
    {
        auto serverSocket = connect_to_server();
        auto firstDisconnect = spawn(client->disconnect());
        auto joinedDisconnect = spawn(client->disconnect());
        run();

        REQUIRE_NOTHROW(firstDisconnect.get());
        REQUIRE_NOTHROW(joinedDisconnect.get());
        REQUIRE(client->state() == network::ConnectionState::Disconnected);
        (void)serverSocket;

        if (cycle == 0)
        {
            restart();
        }
    }
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Remote close racing disconnect completes successfully",
    "[client][network][connection-manager]")
{
    auto serverSocket = connect_to_server();
    auto disconnected = spawn(client->disconnect());
    close_socket(serverSocket);
    run();

    REQUIRE_NOTHROW(disconnected.get());
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client returns typed ping and state snapshot responses",
    "[client][network][request]")
{
    auto serverSocket = connect_to_server();

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto ping = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);
        REQUIRE(ping.request_id() == 1);
        REQUIRE(ping.has_ping());
        REQUIRE(ping.ping().timestamp() == 42);
        co_await write_message(serverSocket, make_pong_response(ping.request_id(), 42));

        auto snapshot = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);
        REQUIRE(snapshot.request_id() == 2);
        REQUIRE(snapshot.has_state_snapshot());

        s2d::protocol::ServerMessage response;
        response.set_request_id(snapshot.request_id());
        response.set_status(s2d::protocol::STATUS_OK);
        response.mutable_state_snapshot()->set_state_json("{\"ready\":true}");
        co_await write_message(serverSocket, std::move(response));
        close_socket(serverSocket);
    });

    auto pong = spawn(client->sendRequest(makePing(42)));
    auto snapshot = spawn(client->sendRequest(s2d::protocol::StateSnapshotRequest{}));
    run();

    REQUIRE_NOTHROW(served.get());
    REQUIRE(pong.get().timestamp() == 42);
    REQUIRE(snapshot.get().state_json() == "{\"ready\":true}");
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client correlates concurrent responses that arrive out of order",
    "[client][network][request]")
{
    auto serverSocket = connect_to_server();

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto first = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);
        auto second = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);

        co_await write_message(
            serverSocket,
            make_pong_response(second.request_id(), second.ping().timestamp()));
        co_await write_message(
            serverSocket,
            make_pong_response(first.request_id(), first.ping().timestamp()));
        close_socket(serverSocket);
    });

    auto first = spawn(client->sendRequest(makePing(11)));
    auto second = spawn(client->sendRequest(makePing(22)));
    run();

    REQUIRE_NOTHROW(served.get());
    REQUIRE(first.get().timestamp() == 11);
    REQUIRE(second.get().timestamp() == 22);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client reports server errors and unexpected response payload",
    "[client][network][request]")
{
    auto serverSocket = connect_to_server();

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto ping = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);
        s2d::protocol::ServerMessage error;
        error.set_request_id(ping.request_id());
        error.set_status(s2d::protocol::STATUS_ERROR);
        error.mutable_error()->set_message("server rejected ping");
        co_await write_message(serverSocket, std::move(error));

        auto snapshot = co_await read_message<s2d::protocol::ClientMessage>(serverSocket);
        co_await write_message(serverSocket, make_pong_response(snapshot.request_id()));
        close_socket(serverSocket);
    });

    auto ping = spawn(client->sendRequest(makePing(7)));
    auto snapshot = spawn(client->sendRequest(s2d::protocol::StateSnapshotRequest{}));
    run();

    REQUIRE_NOTHROW(served.get());
    REQUIRE_THROWS_AS(ping.get(), std::runtime_error);
    REQUIRE_THROWS_AS(snapshot.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Remote close fails pending request and updates client state",
    "[client][network][request][connection-manager]")
{
    auto serverSocket = connect_to_server();
    auto read = spawn(read_message<s2d::protocol::ClientMessage>(serverSocket));
    auto pending = spawn(client->sendRequest(makePing(8)));

    REQUIRE(run_until_ready(read, std::chrono::milliseconds{100}));
    REQUIRE(read.get().has_ping());
    close_socket(serverSocket);

    REQUIRE(run_until_ready(pending, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(pending.get(), std::runtime_error);
    REQUIRE(client->state() == network::ConnectionState::Disconnected);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client enforces request timeout and pending request limit",
    "[client][network][request]")
{
    client = network::Client::create(
        io,
        network::Config{
            .maxPendingRequests = 1,
            .requestTimeout = std::chrono::milliseconds{20},
        });
    auto serverSocket = connect_to_server();

    auto read = spawn(read_message<s2d::protocol::ClientMessage>(serverSocket));
    auto first = spawn(client->sendRequest(makePing(1)));
    REQUIRE(run_until_ready(read, std::chrono::milliseconds{100}));
    REQUIRE(read.get().has_ping());

    auto second = spawn(client->sendRequest(makePing(2)));
    REQUIRE(run_until_ready(second, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(second.get(), std::runtime_error);

    REQUIRE(run_until_ready(first, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(first.get(), std::runtime_error);
}
