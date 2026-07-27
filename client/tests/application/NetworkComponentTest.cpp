#include "network/NetworkComponent.hpp"
#include "shared/tests/network/utils/message_io.hpp"
#include "shared/tests/network/utils/protocol_messages.hpp"

#include <chrono>
#include <future>
#include <optional>
#include <thread>
#include <variant>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
using namespace std::chrono_literals;

template <typename Predicate>
bool waitUntil(Predicate predicate)
{
    for (auto attempt = 0; attempt < 200; ++attempt)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(5ms);
    }
    return false;
}
} // namespace

TEST_CASE(
    "NetworkComponent returns full responses in completion order",
    "[client][application][network]")
{
    boost::asio::io_context serverIo;
    boost::asio::ip::tcp::acceptor acceptor{
        serverIo,
        {boost::asio::ip::address_v4::loopback(), 0}};
    auto const endpoint = acceptor.local_endpoint();

    auto served = boost::asio::co_spawn(
        serverIo,
        [&]() -> boost::asio::awaitable<void> {
            auto socket = co_await acceptor.async_accept(boost::asio::use_awaitable);
            auto ping = co_await s2d::test::network::read_message<
                s2d::protocol::ClientMessage>(socket);
            co_await s2d::test::network::write_message(
                socket,
                s2d::test::network::make_pong_response(
                    ping.request_id(),
                    ping.ping().timestamp()));

            auto snapshot = co_await s2d::test::network::read_message<
                s2d::protocol::ClientMessage>(socket);
            s2d::protocol::ServerMessage response;
            response.set_request_id(snapshot.request_id());
            response.set_status(s2d::protocol::STATUS_OK);
            response.mutable_state_snapshot()->set_state_json("{\"ready\":true}");
            co_await s2d::test::network::write_message(socket, std::move(response));
        },
        boost::asio::use_future);
    std::jthread serverWorker{[&] {
        serverIo.run();
    }};

    s2d::client::NetworkComponent network;
    network.connect(endpoint);
    REQUIRE(waitUntil([&] {
        return network.state() == network::ConnectionState::Connected;
    }));

    s2d::protocol::PingRequest ping;
    ping.set_timestamp(42);
    network.sendRequest(std::move(ping));
    network.sendRequest(s2d::protocol::StateSnapshotRequest{});

    std::deque<s2d::client::NetworkResult> results;
    REQUIRE(waitUntil([&] {
        auto batch = network.drainResults();
        results.insert(
            results.end(),
            std::make_move_iterator(batch.begin()),
            std::make_move_iterator(batch.end()));
        return results.size() == 2;
    }));

    REQUIRE(std::get<s2d::protocol::PongResponse>(results[0]).timestamp() == 42);
    REQUIRE(
        std::get<s2d::protocol::StateSnapshotResponse>(results[1]).state_json() ==
        "{\"ready\":true}");
    REQUIRE_NOTHROW(served.get());
}

TEST_CASE(
    "NetworkComponent reports operation errors through FIFO polling",
    "[client][application][network]")
{
    s2d::client::NetworkComponent network;

    s2d::protocol::PingRequest ping;
    ping.set_timestamp(1);
    network.sendRequest(std::move(ping));
    network.sendRequest(s2d::protocol::StateSnapshotRequest{});

    std::deque<s2d::client::NetworkResult> results;
    REQUIRE(waitUntil([&] {
        auto batch = network.drainResults();
        results.insert(
            results.end(),
            std::make_move_iterator(batch.begin()),
            std::make_move_iterator(batch.end()));
        return results.size() == 2;
    }));

    REQUIRE(
        std::get<s2d::client::NetworkError>(results[0]).operation ==
        s2d::client::NetworkOperation::Ping);
    REQUIRE(
        std::get<s2d::client::NetworkError>(results[1]).operation ==
        s2d::client::NetworkOperation::StateSnapshot);
    REQUIRE(network.drainResults().empty());
}

TEST_CASE(
    "NetworkComponent safely disconnects during destruction",
    "[client][application][network]")
{
    boost::asio::io_context serverIo;
    boost::asio::ip::tcp::acceptor acceptor{
        serverIo,
        {boost::asio::ip::address_v4::loopback(), 0}};

    std::promise<boost::asio::ip::tcp::socket> accepted;
    auto acceptedFuture = accepted.get_future();
    std::jthread acceptWorker{[&] {
        accepted.set_value(acceptor.accept());
    }};
    std::optional<boost::asio::ip::tcp::socket> serverSocket;

    {
        s2d::client::NetworkComponent network;
        network.connect(acceptor.local_endpoint());
        serverSocket.emplace(acceptedFuture.get());
        REQUIRE(waitUntil([&] {
            return network.state() == network::ConnectionState::Connected;
        }));
    }

    REQUIRE(serverSocket->is_open());
}
