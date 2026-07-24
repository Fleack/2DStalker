#include "network/Client.hpp"
#include "shared/logger/logger.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <deque>
#include <exception>
#include <functional>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

namespace
{
namespace ui
{
tgui::Color const background{10, 14, 23};
tgui::Color const surface{17, 24, 39};
tgui::Color const surface2{24, 32, 48};
tgui::Color const border{43, 54, 76};

tgui::Color const text{235, 241, 250};
tgui::Color const muted{148, 163, 184};

tgui::Color const accent{96, 165, 250};
tgui::Color const accentHover{125, 184, 255};
tgui::Color const accentDown{59, 130, 246};

tgui::Color const ok{74, 222, 128};
tgui::Color const warn{251, 191, 36};
tgui::Color const err{248, 113, 113};

tgui::Color const dangerBg{88, 28, 28};
tgui::Color const dangerHover{127, 29, 29};
tgui::Color const dangerDown{153, 27, 27};
} // namespace ui

std::string currentTime()
{
    auto const now = std::chrono::system_clock::now();
    auto const time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};

    localtime_s(&tm, &time);

    std::ostringstream out;
    out << std::put_time(&tm, "%H:%M:%S");
    return out.str();
}

tgui::Label::Ptr makeLabel(
    std::string text,
    unsigned int textSize,
    tgui::Color color)
{
    auto label = tgui::Label::create(std::move(text));
    label->setTextSize(textSize);
    label->getRenderer()->setTextColor(color);
    return label;
}

void stylePanel(
    tgui::Panel::Ptr const& panel,
    tgui::Color background,
    tgui::Color border,
    float radius = 14.f)
{
    auto* renderer = panel->getRenderer();

    renderer->setBackgroundColor(background);
    renderer->setBorderColor(border);
    renderer->setBorders(tgui::Borders{1.f, 1.f, 1.f, 1.f});
    renderer->setRoundedBorderRadius(radius);
}

enum class ButtonKind
{
    Primary,
    Secondary,
    Danger
};

tgui::Button::Ptr makeButton(
    std::string text,
    float y,
    ButtonKind kind = ButtonKind::Secondary)
{
    auto button = tgui::Button::create(std::move(text));

    button->setPosition(24.f, y);
    button->setSize(212.f, 44.f);
    button->setTextSize(15);

    auto* renderer = button->getRenderer();

    renderer->setBorders(tgui::Borders{0.f, 0.f, 0.f, 0.f});
    renderer->setRoundedBorderRadius(10.f);
    renderer->setTextColor(ui::text);

    switch (kind)
    {
    case ButtonKind::Primary:
        renderer->setBackgroundColor(ui::accentDown);
        renderer->setBackgroundColorHover(ui::accent);
        renderer->setBackgroundColorDown(ui::accentHover);
        break;

    case ButtonKind::Secondary:
        renderer->setBackgroundColor(ui::surface2);
        renderer->setBackgroundColorHover(tgui::Color{35, 45, 66});
        renderer->setBackgroundColorDown(tgui::Color{47, 60, 86});
        break;

    case ButtonKind::Danger:
        renderer->setBackgroundColor(ui::dangerBg);
        renderer->setBackgroundColorHover(ui::dangerHover);
        renderer->setBackgroundColorDown(ui::dangerDown);
        break;
    }

    return button;
}

std::string exceptionMessage(std::exception_ptr error)
{
    if (!error)
    {
        return {};
    }

    try
    {
        std::rethrow_exception(error);
    }
    catch (std::exception const& exception)
    {
        return exception.what();
    }
    catch (...)
    {
        return "Unknown client error";
    }
}
} // namespace

using namespace boost;

int main()
{
    asio::io_context net_context;

    auto work = asio::make_work_guard(net_context);

    std::jthread worker([&] {
        net_context.run();
    });

    auto client = network::Client::create(net_context);

    std::mutex uiTasksMutex;
    std::deque<std::move_only_function<void()>> uiTasks;

    auto postUi = [&](std::move_only_function<void()> task) {
        std::scoped_lock lock{uiTasksMutex};
        uiTasks.push_back(std::move(task));
    };

    sf::RenderWindow window(
        sf::VideoMode({1000u, 680u}),
        "TextStalker Client");

    window.setFramerateLimit(60);

    tgui::Gui gui{window};

    // Background accents behind TGUI.
    sf::CircleShape blueGlow{360.f};
    blueGlow.setFillColor(sf::Color{37, 99, 235, 26});
    blueGlow.setPosition({680.f, -220.f});

    sf::CircleShape violetGlow{260.f};
    violetGlow.setFillColor(sf::Color{124, 58, 237, 18});
    violetGlow.setPosition({-150.f, 420.f});

    auto root = tgui::Panel::create();
    root->setPosition(0.f, 0.f);
    root->setSize(1000.f, 680.f);
    root->getRenderer()->setBackgroundColor(tgui::Color::Transparent);

    auto sidebar = tgui::Panel::create();
    sidebar->setPosition(32.f, 32.f);
    sidebar->setSize(260.f, 616.f);
    stylePanel(sidebar, ui::surface, ui::border, 18.f);

    auto appTitle = makeLabel("TextStalker", 24, ui::text);
    appTitle->setPosition(24.f, 24.f);

    auto appSubtitle = makeLabel("Network client", 14, ui::muted);
    appSubtitle->setPosition(24.f, 60.f);

    auto sectionLabel = makeLabel("Actions", 13, ui::muted);
    sectionLabel->setPosition(24.f, 108.f);

    auto connectButton = makeButton("Connect", 136.f, ButtonKind::Primary);
    auto disconnectButton = makeButton("Disconnect", 192.f);
    auto snapshotButton = makeButton("Send state snapshot", 248.f);
    auto pingButton = makeButton("Send ping", 304.f);
    auto quitButton = makeButton("Quit", 548.f, ButtonKind::Danger);

    sidebar->add(appTitle);
    sidebar->add(appSubtitle);
    sidebar->add(sectionLabel);
    sidebar->add(connectButton);
    sidebar->add(disconnectButton);
    sidebar->add(snapshotButton);
    sidebar->add(pingButton);
    sidebar->add(quitButton);

    auto title = makeLabel("Client console", 28, ui::text);
    title->setPosition(324.f, 40.f);

    auto subtitle = makeLabel(
        "Local control panel for protocol requests",
        15,
        ui::muted);
    subtitle->setPosition(326.f, 78.f);

    auto statusCard = tgui::Panel::create();
    statusCard->setPosition(324.f, 120.f);
    statusCard->setSize(644.f, 96.f);
    stylePanel(statusCard, ui::surface, ui::border, 18.f);

    auto statusCaption = makeLabel("Connection status", 13, ui::muted);
    statusCaption->setPosition(24.f, 18.f);

    auto statusDot = tgui::Panel::create();
    statusDot->setPosition(24.f, 54.f);
    statusDot->setSize(12.f, 12.f);
    stylePanel(statusDot, ui::err, ui::err, 6.f);
    statusDot->getRenderer()->setBorders(tgui::Borders{0.f, 0.f, 0.f, 0.f});

    auto statusLabel = makeLabel("Disconnected", 20, ui::text);
    statusLabel->setPosition(46.f, 47.f);

    auto endpointLabel = makeLabel("127.0.0.1:1234", 14, ui::muted);
    endpointLabel->setPosition(470.f, 50.f);

    statusCard->add(statusCaption);
    statusCard->add(statusDot);
    statusCard->add(statusLabel);
    statusCard->add(endpointLabel);

    auto logTitle = makeLabel("Event log", 18, ui::text);
    logTitle->setPosition(324.f, 244.f);

    auto logHint = makeLabel("Recent client activity", 13, ui::muted);
    logHint->setPosition(324.f, 270.f);

    auto logView = tgui::TextArea::create();
    logView->setPosition(324.f, 304.f);
    logView->setSize(644.f, 344.f);
    logView->setTextSize(14);
    logView->setReadOnly(true);

    auto* logRenderer = logView->getRenderer();
    logRenderer->setBackgroundColor(ui::surface);
    logRenderer->setTextColor(ui::text);
    logRenderer->setDefaultTextColor(ui::muted);
    logRenderer->setBorderColor(ui::border);
    logRenderer->setBorders(tgui::Borders{1.f, 1.f, 1.f, 1.f});
    logRenderer->setPadding(tgui::Padding{16.f, 14.f, 16.f, 14.f});
    logRenderer->setRoundedBorderRadius(18.f);
    logRenderer->setCaretColor(tgui::Color::Transparent);

    std::deque<std::string> logLines;

    auto refreshLog = [&] {
        std::string text;

        for (auto const& line : logLines)
        {
            text += line;
            text += '\n';
        }

        logView->setText(text);
        logView->getVerticalScrollbar()->setValue(logView->getVerticalScrollbar()->getMaxValue());
    };

    auto addLog = [&](std::string const& line) {
        constexpr std::size_t maxLines = 80;

        if (logLines.size() >= maxLines)
        {
            logLines.pop_front();
        }

        std::string entry;
        entry.reserve(line.size() + 16);
        entry += "[";
        entry += currentTime();
        entry += "] ";
        entry += line;

        logLines.push_back(std::move(entry));
        refreshLog();
    };

    auto setStatus = [&](std::string const& text, tgui::Color color) {
        statusLabel->setText(text);
        statusDot->getRenderer()->setBackgroundColor(color);
        statusDot->getRenderer()->setBorderColor(color);
    };

    addLog("Ready");

    auto shownState = network::ConnectionState::Disconnected;
    auto showConnectionState = [&](network::ConnectionState state) {
        shownState = state;

        auto const connected = state == network::ConnectionState::Connected;
        connectButton->setEnabled(state == network::ConnectionState::Disconnected);
        disconnectButton->setEnabled(
            state == network::ConnectionState::Connecting || connected);
        pingButton->setEnabled(connected);
        snapshotButton->setEnabled(connected);

        switch (state)
        {
        case network::ConnectionState::Disconnected:
            setStatus("Disconnected", ui::err);
            break;
        case network::ConnectionState::Connecting:
            setStatus("Connecting...", ui::warn);
            break;
        case network::ConnectionState::Connected:
            setStatus("Connected", ui::ok);
            break;
        case network::ConnectionState::Disconnecting:
            setStatus("Disconnecting...", ui::warn);
            break;
        }
    };

    showConnectionState(shownState);

    connectButton->onPress([&] {
        connectButton->setEnabled(false);

        auto const ip = asio::ip::make_address("127.0.0.1");
        constexpr std::uint16_t port = 1234;

        addLog("Connecting to 127.0.0.1:1234");

        asio::co_spawn(
            net_context,
            client->connect(ip, port),
            [&](std::exception_ptr error) {
                postUi([&, message = exceptionMessage(error)] {
                    showConnectionState(client->state());

                    if (message.empty())
                    {
                        addLog("Connected");
                        return;
                    }

                    addLog("Connect failed: " + message);
                    LOG(err, "Client connect failed: {}", message);
                });
            });
    });

    disconnectButton->onPress([&] {
        disconnectButton->setEnabled(false);
        addLog("Disconnect requested");

        asio::co_spawn(
            net_context,
            client->disconnect(),
            [&](std::exception_ptr error) {
                postUi([&, message = exceptionMessage(error)] {
                    showConnectionState(client->state());

                    if (message.empty())
                    {
                        addLog("Disconnected");
                        return;
                    }

                    addLog("Disconnect failed: " + message);
                    LOG(err, "Client disconnect failed: {}", message);
                });
            });
    });

    snapshotButton->onPress([&] {
        addLog("Sending state snapshot");

        asio::co_spawn(
            net_context,
            client->sendRequest(s2d::protocol::StateSnapshotRequest{}),
            [&](std::exception_ptr error, s2d::protocol::StateSnapshotResponse response) {
                postUi([&, message = exceptionMessage(error), response = std::move(response)] {
                    if (message.empty())
                    {
                        addLog("State snapshot: " + response.state_json());
                        return;
                    }

                    addLog("State snapshot failed: " + message);
                    LOG(err, "State snapshot failed: {}", message);
                });
            });
    });

    pingButton->onPress([&] {
        s2d::protocol::PingRequest request;
        request.set_timestamp(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
        addLog("Sending ping");

        asio::co_spawn(
            net_context,
            client->sendRequest(std::move(request)),
            [&](std::exception_ptr error, s2d::protocol::PongResponse response) {
                postUi([&, message = exceptionMessage(error), response = std::move(response)] {
                    if (message.empty())
                    {
                        addLog("Pong: " + std::to_string(response.timestamp()));
                        return;
                    }

                    addLog("Ping failed: " + message);
                    LOG(err, "Ping failed: {}", message);
                });
            });
    });

    auto closeApp = [&] {
        window.close();
    };

    quitButton->onPress(closeApp);

    root->add(sidebar);
    root->add(title);
    root->add(subtitle);
    root->add(statusCard);
    root->add(logTitle);
    root->add(logHint);
    root->add(logView);

    gui.add(root);

    while (window.isOpen())
    {
        while (std::optional event = window.pollEvent())
        {
            gui.handleEvent(*event);

            if (event->is<sf::Event::Closed>())
            {
                closeApp();
            }
        }

        std::deque<std::move_only_function<void()>> pendingUiTasks;
        {
            std::scoped_lock lock{uiTasksMutex};
            pendingUiTasks.swap(uiTasks);
        }
        for (auto& task : pendingUiTasks)
        {
            task();
        }

        auto const currentState = client->state();
        if (currentState != shownState)
        {
            showConnectionState(currentState);
        }

        window.clear(sf::Color{10, 14, 23});
        window.draw(blueGlow);
        window.draw(violetGlow);

        gui.draw();

        window.display();
    }

    auto shutdown = asio::co_spawn(
        net_context,
        [client]() -> asio::awaitable<void> {
            while (client->state() != network::ConnectionState::Disconnected)
            {
                auto const state = client->state();
                if (state == network::ConnectionState::Connecting ||
                    state == network::ConnectionState::Connected)
                {
                    try
                    {
                        co_await client->disconnect();
                    }
                    catch (std::logic_error const&)
                    {
                    }
                    continue;
                }

                asio::steady_timer wait{co_await asio::this_coro::executor};
                wait.expires_after(std::chrono::milliseconds{10});
                co_await wait.async_wait(asio::use_awaitable);
            }
        },
        asio::use_future);

    shutdown.get();
    work.reset();
    worker.join();

    return 0;
}
