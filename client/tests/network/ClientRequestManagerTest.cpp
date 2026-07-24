#include "network/ClientRequestManager.hpp"
#include "shared/tests/network/utils/message_io.hpp"
#include "shared/tests/network/utils/network_fixtures.hpp"
#include "shared/tests/network/utils/protocol_messages.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <catch2/catch_test_macros.hpp>

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

using namespace boost;

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "ClientRequestManager packs typed request and ignores unknown response id",
    "[client][network][request-manager]")
{
    auto manager = std::make_shared<network::ClientRequestManager>(
        asio::make_strand(io),
        network::Config{});
    manager->start(std::move(sockets.client), [] {});

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto request = co_await read_message<s2d::protocol::ClientMessage>(sockets.server);
        REQUIRE(request.request_id() == 1);
        REQUIRE(request.has_ping());
        REQUIRE(request.ping().timestamp() == 55);

        co_await write_message(sockets.server, make_pong_response(999, 1));
        co_await write_message(sockets.server, make_pong_response(request.request_id(), request.ping().timestamp()));
    });
    auto response = spawn(manager->sendRequest(makePing(55)));

    REQUIRE(run_until_ready(served, std::chrono::milliseconds{100}));
    REQUIRE(run_until_ready(response, std::chrono::milliseconds{100}));

    REQUIRE_NOTHROW(served.get());
    REQUIRE(response.get().timestamp() == 55);

    restart();
    manager->stop("test complete");
    run();
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "ClientRequestManager stop fails pending request before reporting close",
    "[client][network][request-manager]")
{
    auto manager = std::make_shared<network::ClientRequestManager>(
        asio::make_strand(io),
        network::Config{
            .requestTimeout = std::chrono::seconds{1},
        });
    manager->start(std::move(sockets.client), []() {});

    auto received = spawn(read_message<s2d::protocol::ClientMessage>(sockets.server));
    auto pending = spawn(manager->sendRequest(makePing(10)));
    REQUIRE(run_until_ready(received, std::chrono::milliseconds{100}));
    REQUIRE(received.get().has_ping());

    manager->stop("local disconnect");
    run();

    REQUIRE_THROWS_AS(pending.get(), std::runtime_error);
}

TEST_CASE_METHOD(
    s2d::test::network::socket_pair_fixture,
    "ClientRequestManager validates typed response payload",
    "[client][network][request-manager]")
{
    auto manager = std::make_shared<network::ClientRequestManager>(
        asio::make_strand(io),
        network::Config{});
    (void)manager->start(
        std::move(sockets.client), [] {});

    auto served = spawn([&]() -> asio::awaitable<void> {
        auto request = co_await read_message<s2d::protocol::ClientMessage>(sockets.server);
        s2d::protocol::ServerMessage response;
        response.set_request_id(request.request_id());
        response.set_status(s2d::protocol::STATUS_ERROR);
        response.mutable_error()->set_message("request rejected");
        co_await write_message(sockets.server, std::move(response));
    });
    auto response = spawn(manager->sendRequest(makePing(1)));

    REQUIRE(run_until_ready(served, std::chrono::milliseconds{100}));
    REQUIRE(run_until_ready(response, std::chrono::milliseconds{100}));

    REQUIRE_NOTHROW(served.get());
    REQUIRE_THROWS_AS(response.get(), std::runtime_error);

    manager->stop("test complete");
    run();
}
