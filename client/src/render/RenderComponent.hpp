#pragma once

#include <optional>
#include <string_view>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>

namespace s2d::client
{
class RenderComponent
{
public:
    RenderComponent(sf::VideoMode mode, std::string_view title, unsigned int frameLimit);

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] std::optional<sf::Event> pollEvent();
    [[nodiscard]] sf::Time nextFrameTime();

    void beginFrame();
    [[nodiscard]] sf::RenderTarget& target();
    void present();
    void close();

    [[nodiscard]] sf::RenderWindow& window();

private:
    sf::RenderWindow m_window;
    sf::Clock m_frameClock;
};
} // namespace s2d::client
