#include "RenderComponent.hpp"

#include <string>

#include <SFML/Graphics/Color.hpp>

namespace
{
constexpr sf::Color clearColor{10, 14, 23};
}

namespace s2d::client
{
RenderComponent::RenderComponent(
    sf::VideoMode mode,
    std::string_view title,
    unsigned int frameLimit)
    : m_window{mode, std::string{title}}
{
    m_window.setFramerateLimit(frameLimit);
}

bool RenderComponent::isOpen() const
{
    return m_window.isOpen();
}

std::optional<sf::Event> RenderComponent::pollEvent()
{
    return m_window.pollEvent();
}

sf::Time RenderComponent::nextFrameTime()
{
    return m_frameClock.restart();
}

void RenderComponent::beginFrame()
{
    m_window.clear(clearColor);
}

sf::RenderTarget& RenderComponent::target()
{
    return m_window;
}

void RenderComponent::present()
{
    m_window.display();
}

void RenderComponent::close()
{
    m_window.close();
}

sf::RenderWindow& RenderComponent::window()
{
    return m_window;
}
} // namespace s2d::client
