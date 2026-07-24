#pragma once

#include "ClientTypes.hpp"
#include "shared/protocol/message.pb.h"

#include <cstdint>
#include <memory>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>

namespace network
{
using namespace boost;

class ConnectionManager;

class ClientRequestManager;

class Client
{
public:
    static std::shared_ptr<Client> create(asio::io_context& io, Config config = {});

    ~Client();

    Client(Client const&) = delete;
    Client& operator=(Client const&) = delete;

    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    asio::awaitable<void> connect(asio::ip::address address, std::uint16_t port);
    asio::awaitable<void> disconnect();

    asio::awaitable<s2d::protocol::PongResponse> sendRequest(s2d::protocol::PingRequest request);
    asio::awaitable<s2d::protocol::StateSnapshotResponse> sendRequest(s2d::protocol::StateSnapshotRequest request);

    [[nodiscard]] ConnectionState state() const noexcept;

private:
    Client(std::shared_ptr<ClientRequestManager> requestsManager, std::shared_ptr<ConnectionManager> connectionManager) noexcept;

private:
    std::shared_ptr<ClientRequestManager> m_requestsManager;
    std::shared_ptr<ConnectionManager> m_connectionManager;
};
} // namespace network
