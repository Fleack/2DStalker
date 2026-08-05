#pragma once

#include "../network/ClientTypes.hpp"

#include <functional>
#include <string>

#include <TGUI/Widgets/Button.hpp>
#include <TGUI/Widgets/Label.hpp>
#include <TGUI/Widgets/Panel.hpp>

namespace s2d::client
{
class ConnectionPanel
{
public:
    ConnectionPanel(
        tgui::Panel& root,
        std::string endpointText,
        std::function<void()> connect,
        std::function<void()> disconnect,
        std::function<void()> snapshot,
        std::function<void()> ping,
        std::function<void()> quit);

    void showState(network::ConnectionState state);
    void disableConnect();
    void disableDisconnect();

private:
    tgui::Panel::Ptr m_statusDot;
    tgui::Label::Ptr m_statusLabel;
    tgui::Button::Ptr m_connectButton;
    tgui::Button::Ptr m_disconnectButton;
    tgui::Button::Ptr m_snapshotButton;
    tgui::Button::Ptr m_pingButton;
};
} // namespace s2d::client
