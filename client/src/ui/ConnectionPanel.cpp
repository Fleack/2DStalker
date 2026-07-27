#include "ConnectionPanel.hpp"

#include "Style.hpp"

#include <utility>

namespace s2d::client
{
ConnectionPanel::ConnectionPanel(
    tgui::Panel& root,
    std::string endpointText,
    std::function<void()> connect,
    std::function<void()> disconnect,
    std::function<void()> snapshot,
    std::function<void()> ping,
    std::function<void()> quit)
    : m_statusDot{tgui::Panel::create()}
    , m_statusLabel{ui::style::makeLabel("Disconnected", 20, ui::style::text)}
    , m_connectButton{
          ui::style::makeButton("Connect", 136.F, ui::style::ButtonKind::Primary)}
    , m_disconnectButton{ui::style::makeButton("Disconnect", 192.F)}
    , m_snapshotButton{ui::style::makeButton("Send state snapshot", 248.F)}
    , m_pingButton{ui::style::makeButton("Send ping", 304.F)}
{
    auto sidebar = tgui::Panel::create();
    sidebar->setPosition(32.F, 32.F);
    sidebar->setSize(260.F, 616.F);
    ui::style::stylePanel(sidebar, ui::style::surface, ui::style::border, 18.F);

    auto appTitle = ui::style::makeLabel("TextStalker", 24, ui::style::text);
    appTitle->setPosition(24.F, 24.F);
    auto appSubtitle =
        ui::style::makeLabel("Network client", 14, ui::style::muted);
    appSubtitle->setPosition(24.F, 60.F);
    auto sectionLabel = ui::style::makeLabel("Actions", 13, ui::style::muted);
    sectionLabel->setPosition(24.F, 108.F);
    auto quitButton =
        ui::style::makeButton("Quit", 548.F, ui::style::ButtonKind::Danger);

    m_connectButton->onPress(std::move(connect));
    m_disconnectButton->onPress(std::move(disconnect));
    m_snapshotButton->onPress(std::move(snapshot));
    m_pingButton->onPress(std::move(ping));
    quitButton->onPress(std::move(quit));

    sidebar->add(appTitle);
    sidebar->add(appSubtitle);
    sidebar->add(sectionLabel);
    sidebar->add(m_connectButton);
    sidebar->add(m_disconnectButton);
    sidebar->add(m_snapshotButton);
    sidebar->add(m_pingButton);
    sidebar->add(quitButton);

    auto title = ui::style::makeLabel("Client console", 28, ui::style::text);
    title->setPosition(324.F, 40.F);
    auto subtitle = ui::style::makeLabel(
        "Local control panel for protocol requests",
        15,
        ui::style::muted);
    subtitle->setPosition(326.F, 78.F);

    auto statusCard = tgui::Panel::create();
    statusCard->setPosition(324.F, 120.F);
    statusCard->setSize(644.F, 96.F);
    ui::style::stylePanel(
        statusCard,
        ui::style::surface,
        ui::style::border,
        18.F);

    auto statusCaption =
        ui::style::makeLabel("Connection status", 13, ui::style::muted);
    statusCaption->setPosition(24.F, 18.F);
    m_statusDot->setPosition(24.F, 54.F);
    m_statusDot->setSize(12.F, 12.F);
    ui::style::stylePanel(m_statusDot, ui::style::err, ui::style::err, 6.F);
    m_statusDot->getRenderer()->setBorders(tgui::Borders{0.F});
    m_statusLabel->setPosition(46.F, 47.F);
    auto endpointLabel =
        ui::style::makeLabel(std::move(endpointText), 14, ui::style::muted);
    endpointLabel->setPosition(470.F, 50.F);

    statusCard->add(statusCaption);
    statusCard->add(m_statusDot);
    statusCard->add(m_statusLabel);
    statusCard->add(endpointLabel);

    root.add(sidebar);
    root.add(title);
    root.add(subtitle);
    root.add(statusCard);
}

void ConnectionPanel::showState(network::ConnectionState state)
{
    auto const connected = state == network::ConnectionState::Connected;
    m_connectButton->setEnabled(state == network::ConnectionState::Disconnected);
    m_disconnectButton->setEnabled(
        state == network::ConnectionState::Connecting || connected);
    m_pingButton->setEnabled(connected);
    m_snapshotButton->setEnabled(connected);

    tgui::Color color;
    switch (state)
    {
    case network::ConnectionState::Disconnected:
        m_statusLabel->setText("Disconnected");
        color = ui::style::err;
        break;
    case network::ConnectionState::Connecting:
        m_statusLabel->setText("Connecting...");
        color = ui::style::warn;
        break;
    case network::ConnectionState::Connected:
        m_statusLabel->setText("Connected");
        color = ui::style::ok;
        break;
    case network::ConnectionState::Disconnecting:
        m_statusLabel->setText("Disconnecting...");
        color = ui::style::warn;
        break;
    }

    m_statusDot->getRenderer()->setBackgroundColor(color);
    m_statusDot->getRenderer()->setBorderColor(color);
}

void ConnectionPanel::disableConnect()
{
    m_connectButton->setEnabled(false);
}

void ConnectionPanel::disableDisconnect()
{
    m_disconnectButton->setEnabled(false);
}
} // namespace s2d::client
