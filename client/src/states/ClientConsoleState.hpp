#pragma once

#include "states/ApplicationState.hpp"
#include "ui/ConnectionPanel.hpp"
#include "ui/EventLogPanel.hpp"

#include <array>
#include <functional>

#include <SFML/Graphics/CircleShape.hpp>

namespace s2d::client
{
class ClientConsoleState final : public ApplicationState
{
public:
    ClientConsoleState(
        NetworkComponent& network,
        boost::asio::ip::tcp::endpoint endpoint,
        std::function<void()> onQuit);

    [[nodiscard]] tgui::Widget::Ptr uiRoot() const noexcept override;

    void update(sf::Time frameTime) override;
    void render(sf::RenderTarget& target) override;

    void handleEvent(sf::Event const&) override {}
    void handleNetworkResult(NetworkResult const& result) override;

    [[nodiscard]] bool drawsBelow() const noexcept override
    {
        return false;
    }
    [[nodiscard]] bool updatesBelow() const noexcept override
    {
        return false;
    }

private:
    NetworkComponent& m_network;
    tgui::Panel::Ptr m_root;
    ConnectionPanel m_connectionPanel;
    EventLogPanel m_eventLog;
    std::array<sf::CircleShape, 2> m_background;
    network::ConnectionState m_shownState;
};
} // namespace s2d::client
