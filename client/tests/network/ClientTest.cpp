#include "network/Client.hpp"
#include "shared/tests/network/utils/message_io.hpp"
#include "shared/tests/network/utils/protocol_messages.hpp"
#include "shared/tests/utils/future_assertions.hpp"
#include "shared/tests/utils/protobuf_assertions.hpp"
#include "utils/client_network_fixture.hpp"

#include <chrono>
#include <stdexcept>
#include <system_error>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <catch2/catch_test_macros.hpp>

using s2d::test::require_messages_equal;
using s2d::test::require_ready;
using s2d::test::network::available_bytes_after_wait;
using s2d::test::network::close_socket;
using s2d::test::network::make_ping_request;
using s2d::test::network::make_pong_response;
using s2d::test::network::read_message;
using s2d::test::network::write_message;

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client send throws when disconnected",
    "[client][network]")
{
    auto sent = spawn(client->send(make_ping_request(1)));

    run();

    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client failed connect leaves client disconnected",
    "[client][network]")
{
    asio::ip::tcp::endpoint const endpoint{asio::ip::address_v4::loopback(), 0};

    auto connected = spawn(client->connect(endpoint.address(), endpoint.port()));

    run();
    REQUIRE_THROWS_AS(connected.get(), std::system_error);

    restart();
    auto sent = spawn(client->send(make_ping_request(2)));

    run();
    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client rejects duplicate connect while connected",
    "[client][network]")
{
    auto server_socket = connect_to_server();
    (void)server_socket;

    auto connected = spawn(client->connect(asio::ip::address_v4::loopback(), 1));

    REQUIRE(run_until_ready(connected, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(connected.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client returns matching response without sending response to response",
    "[client][network]")
{
    auto server_socket = connect_to_server();

    auto request = make_ping_request(0);
    auto expected_request = make_ping_request(1);
    auto response = make_pong_response(expected_request.request_id());

    auto served = spawn([&]() -> asio::awaitable<std::size_t> {
        auto received_request = co_await read_message<s2d::protocol::ClientMessage>(server_socket);
        require_messages_equal(expected_request, received_request);

        co_await write_message(server_socket, response);

        auto const available_bytes = co_await available_bytes_after_wait(server_socket);
        close_socket(server_socket);
        co_return available_bytes;
    });
    auto sent = spawn(client->send(request));

    run();

    require_ready(served);
    require_ready(sent);
    REQUIRE(served.get() == 0);
    require_messages_equal(response, sent.get());
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client correlates concurrent responses by request id",
    "[client][network]")
{
    auto server_socket = connect_to_server();

    auto first_request = make_ping_request(0);
    auto second_request = make_ping_request(0);
    auto expected_first_request = make_ping_request(1);
    auto expected_second_request = make_ping_request(2);

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto first_received = co_await read_message<s2d::protocol::ClientMessage>(server_socket);
        auto second_received = co_await read_message<s2d::protocol::ClientMessage>(server_socket);

        require_messages_equal(expected_first_request, first_received);
        require_messages_equal(expected_second_request, second_received);

        co_await write_message(server_socket, make_pong_response(expected_second_request.request_id()));
        co_await write_message(server_socket, make_pong_response(expected_first_request.request_id()));

        close_socket(server_socket);
        co_return;
    });
    auto first_sent = spawn(client->send(first_request));
    auto second_sent = spawn(client->send(second_request));

    run();

    REQUIRE_NOTHROW(served.get());
    require_messages_equal(make_pong_response(expected_first_request.request_id()), first_sent.get());
    require_messages_equal(make_pong_response(expected_second_request.request_id()), second_sent.get());
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client rejects send when pending request limit is reached",
    "[client][network]")
{
    client = Client::create(io, Client::Config{.maxPendingRequests = 1});
    auto server_socket = connect_to_server();

    auto request = make_ping_request(0);
    auto expected_request = make_ping_request(1);
    auto response = make_pong_response(expected_request.request_id());

    auto read = spawn(read_message<s2d::protocol::ClientMessage>(server_socket));
    auto first_sent = spawn(client->send(request));

    REQUIRE(run_until_ready(read, std::chrono::milliseconds{100}));
    require_messages_equal(expected_request, read.get());

    auto second_sent = spawn(client->send(request));

    REQUIRE(run_until_ready(second_sent, std::chrono::milliseconds{100}));
    REQUIRE_THROWS_AS(second_sent.get(), std::runtime_error);

    auto write = spawn([&]() -> asio::awaitable<void> {
        co_await write_message(server_socket, response);
        close_socket(server_socket);
        co_return;
    });

    run();

    REQUIRE_NOTHROW(write.get());
    require_messages_equal(response, first_sent.get());
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client closes pending request on unexpected response id",
    "[client][network]")
{
    auto server_socket = connect_to_server();

    auto request = make_ping_request(0);
    auto expected_request = make_ping_request(1);
    auto wrong_response = make_pong_response(2);

    auto served = spawn([&]() -> asio::awaitable<std::size_t> {
        auto received_request = co_await read_message<s2d::protocol::ClientMessage>(server_socket);
        require_messages_equal(expected_request, received_request);

        co_await write_message(server_socket, wrong_response);

        auto const available_bytes = co_await available_bytes_after_wait(server_socket);
        close_socket(server_socket);
        co_return available_bytes;
    });
    auto sent = spawn(client->send(request));

    run();

    REQUIRE(served.get() == 0);
    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client disconnect cancels pending request",
    "[client][network]")
{
    auto server_socket = connect_to_server();

    auto request = make_ping_request(0);
    auto expected_request = make_ping_request(1);

    auto read = spawn(read_message<s2d::protocol::ClientMessage>(server_socket));
    auto sent = spawn(client->send(request));

    REQUIRE(run_until_ready(read, std::chrono::milliseconds{100}));
    require_messages_equal(expected_request, read.get());

    client->disconnect();
    run();

    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client server-side disconnect cancels pending request",
    "[client][network]")
{
    auto server_socket = connect_to_server();

    auto request = make_ping_request(0);
    auto expected_request = make_ping_request(1);

    auto read = spawn(read_message<s2d::protocol::ClientMessage>(server_socket));
    auto sent = spawn(client->send(request));

    REQUIRE(run_until_ready(read, std::chrono::milliseconds{100}));
    require_messages_equal(expected_request, read.get());

    close_socket(server_socket);
    REQUIRE(run_until_ready(sent, std::chrono::milliseconds{100}));

    require_ready(sent);
    REQUIRE_THROWS_AS(sent.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::client::network::client_network_fixture,
    "Client disconnect is idempotent",
    "[client][network]")
{
    REQUIRE_NOTHROW(client->disconnect());
    REQUIRE_NOTHROW(client->disconnect());
}
