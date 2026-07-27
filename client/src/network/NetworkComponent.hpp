#pragma once

#include "Client.hpp"

#include <deque>
#include <mutex>
#include <thread>
#include <variant>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace s2d::client
{
enum class NetworkOperation
{
    Connect,
    Disconnect,
    Ping,
    StateSnapshot,
};

struct NetworkError
{
    NetworkOperation operation;
    std::string message;
};

using NetworkResult = std::variant<
    s2d::protocol::PongResponse,
    s2d::protocol::StateSnapshotResponse,
    NetworkError>;

class NetworkComponent
{
public:
    explicit NetworkComponent(network::Config config = {});
    ~NetworkComponent();

    NetworkComponent(NetworkComponent const&) = delete;
    NetworkComponent& operator=(NetworkComponent const&) = delete;
    NetworkComponent(NetworkComponent&&) = delete;
    NetworkComponent& operator=(NetworkComponent&&) = delete;

    void connect(boost::asio::ip::tcp::endpoint endpoint);
    void disconnect();

    void sendRequest(s2d::protocol::PingRequest request);
    void sendRequest(s2d::protocol::StateSnapshotRequest request);

    [[nodiscard]] network::ConnectionState state() const noexcept;
    [[nodiscard]] std::deque<NetworkResult> drainResults();

private:
    using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    void publish(NetworkResult result);

private:
    boost::asio::io_context m_io;
    work_guard_type m_work;
    std::shared_ptr<network::Client> m_client;
    std::mutex m_resultsMutex;
    std::deque<NetworkResult> m_results;
    std::jthread m_worker;
};
} // namespace s2d::client
