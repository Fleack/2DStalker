#include "UiComponent.hpp"

namespace s2d::client
{
UiComponent::UiComponent(sf::RenderWindow& window)
    : m_gui{window}
{
}

UiComponent::UiComponent(sf::RenderTarget& target)
    : m_gui{target}
{
}

bool UiComponent::handleEvent(sf::Event const& event)
{
    return m_gui.handleEvent(event);
}

void UiComponent::mount(tgui::Widget::Ptr const& root)
{
    m_gui.add(root);
}

void UiComponent::unmount(tgui::Widget::Ptr const& root)
{
    static_cast<void>(m_gui.remove(root));
}

void UiComponent::setInteractive(tgui::Widget::Ptr const& root, bool interactive)
{
    root->setEnabled(interactive);
}

void UiComponent::draw()
{
    m_gui.draw();
}
} // namespace s2d::client
