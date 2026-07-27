#include "states/ClientConsoleState.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
std::string endpointText(boost::asio::ip::tcp::endpoint const& endpoint)
{
    return endpoint.address().to_string() + ':' + std::to_string(endpoint.port());
}

std::string operationName(s2d::client::NetworkOperation operation)
{
    using s2d::client::NetworkOperation;
    switch (operation)
    {
    case NetworkOperation::Connect:
        return "Connect";
    case NetworkOperation::Disconnect:
        return "Disconnect";
    case NetworkOperation::Ping:
        return "Ping";
    case NetworkOperation::StateSnapshot:
        return "State snapshot";
    }
    return "Network operation";
}
} // namespace

namespace s2d::client
{
ClientConsoleState::ClientConsoleState(
    NetworkComponent& network,
    boost::asio::ip::tcp::endpoint endpoint,
    std::function<void()> onQuit)
    : m_network{network}
    , m_root{tgui::Panel::create()}
    , m_connectionPanel{
          *m_root,
          endpointText(endpoint),
          [this, endpoint] {
              m_connectionPanel.disableConnect();
              m_eventLog.add("Connecting to " + endpointText(endpoint));
              m_network.connect(endpoint);
          },
          [this] {
              m_connectionPanel.disableDisconnect();
              m_eventLog.add("Disconnect requested");
              m_network.disconnect();
          },
          [this] {
              m_eventLog.add("Sending state snapshot");
              m_network.sendRequest(s2d::protocol::StateSnapshotRequest{});
          },
          [this] {
              s2d::protocol::PingRequest request;
              auto const timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::system_clock::now()
                                             .time_since_epoch())
                                         .count();
              request.set_timestamp(static_cast<std::uint64_t>(timestamp));
              m_eventLog.add("Sending ping");
              m_network.sendRequest(std::move(request));
          },
          std::move(onQuit)}
    , m_eventLog{*m_root}
    , m_background{sf::CircleShape{360.F}, sf::CircleShape{260.F}}
    , m_shownState{m_network.state()}
{
    m_root->setPosition(0.F, 0.F);
    m_root->setSize(1000.F, 680.F);
    m_root->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    m_background[0].setFillColor(sf::Color{37, 99, 235, 26});
    m_background[0].setPosition({680.F, -220.F});
    m_background[1].setFillColor(sf::Color{124, 58, 237, 18});
    m_background[1].setPosition({-150.F, 420.F});

    m_connectionPanel.showState(m_shownState);
    m_eventLog.add("Ready");
}

tgui::Widget::Ptr ClientConsoleState::uiRoot() const noexcept
{
    return m_root;
}

void ClientConsoleState::update(sf::Time)
{
    auto const currentState = m_network.state();
    if (currentState == m_shownState)
    {
        return;
    }

    m_shownState = currentState;
    m_connectionPanel.showState(currentState);

    if (currentState == network::ConnectionState::Connected)
    {
        m_eventLog.add("Connected");
    }
    else if (currentState == network::ConnectionState::Disconnected)
    {
        m_eventLog.add("Disconnected");
    }
}

void ClientConsoleState::render(sf::RenderTarget& target)
{
    for (auto const& shape : m_background)
    {
        target.draw(shape);
    }
}

void ClientConsoleState::handleNetworkResult(NetworkResult const& result)
{
    std::visit(
        [this](auto const& value) {
            using Value = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, s2d::protocol::PongResponse>)
            {
                m_eventLog.add("Pong: " + std::to_string(value.timestamp()));
            }
            else if constexpr (
                std::is_same_v<Value, s2d::protocol::StateSnapshotResponse>)
            {
                m_eventLog.add("State snapshot: " + value.state_json());
            }
            else
            {
                if (value.operation == NetworkOperation::Connect ||
                    value.operation == NetworkOperation::Disconnect)
                {
                    m_shownState = m_network.state();
                    m_connectionPanel.showState(m_shownState);
                }
                m_eventLog.add(
                    operationName(value.operation) + " failed: " + value.message);
            }
        },
        result);
}
} // namespace s2d::client
