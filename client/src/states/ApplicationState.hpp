#pragma once

#include "../network/NetworkComponent.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <TGUI/Widget.hpp>

namespace s2d::client
{
class ApplicationState
{
public:
    virtual ~ApplicationState() = default;

    [[nodiscard]] virtual tgui::Widget::Ptr uiRoot() const noexcept = 0;

    virtual void update(sf::Time frameTime) = 0;
    virtual void render(sf::RenderTarget& target) = 0;

    virtual void handleEvent(sf::Event const&)
    {
    }

    virtual void handleNetworkResult(NetworkResult const&)
    {
    }

    [[nodiscard]] virtual bool drawsBelow() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual bool updatesBelow() const noexcept
    {
        return false;
    }
};
} // namespace s2d::client
