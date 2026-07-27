#pragma once

#include <SFML/Window/Event.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <TGUI/TGUI.hpp>

namespace sf
{
class RenderTarget;
class RenderWindow;
} // namespace sf

namespace s2d::client
{
class UiComponent
{
public:
    explicit UiComponent(sf::RenderWindow& window);
    explicit UiComponent(sf::RenderTarget& target);

    [[nodiscard]] bool handleEvent(sf::Event const& event);

    void mount(tgui::Widget::Ptr const& root);
    void unmount(tgui::Widget::Ptr const& root);
    void setInteractive(tgui::Widget::Ptr const& root, bool interactive);

    void draw();

private:
    tgui::Gui m_gui;
};
} // namespace s2d::client
