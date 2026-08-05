#include "Client.hpp"

#include "ClientRequestManager.hpp"
#include "ConnectionManager.hpp"

#include <stdexcept>
#include <utility>

#include <boost/asio/strand.hpp>

namespace network
{
std::shared_ptr<Client> Client::create(asio::io_context& io, Config config)
{
    config.validate();

    auto strand = asio::make_strand(io);
    auto requestsManager = std::make_shared<ClientRequestManager>(strand, config);
    auto connectionManager = std::make_shared<ConnectionManager>(strand, config, requestsManager);

    return std::shared_ptr<Client>{new Client{std::move(requestsManager), std::move(connectionManager)}};
}

Client::Client(std::shared_ptr<ClientRequestManager> requestsManager, std::shared_ptr<ConnectionManager> connectionManager) noexcept
    : m_requestsManager{std::move(requestsManager)}
    , m_connectionManager{std::move(connectionManager)}
{
}

Client::~Client() = default;

asio::awaitable<void> Client::connect(asio::ip::address address, std::uint16_t port)
{
    return m_connectionManager->connect({address, port});
}

asio::awaitable<void> Client::disconnect()
{
    return m_connectionManager->disconnect();
}

asio::awaitable<s2d::protocol::PongResponse> Client::sendRequest(s2d::protocol::PingRequest request)
{
    return m_requestsManager->sendRequest(std::move(request));
}

asio::awaitable<s2d::protocol::StateSnapshotResponse> Client::sendRequest(s2d::protocol::StateSnapshotRequest request)
{
    return m_requestsManager->sendRequest(std::move(request));
}

ConnectionState Client::state() const noexcept
{
    return m_connectionManager->state();
}
} // namespace network
