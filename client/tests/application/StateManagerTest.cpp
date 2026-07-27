#include "states/StateManager.hpp"
#include "ui/UiComponent.hpp"

#include <functional>
#include <memory>

#include <SFML/Graphics/RenderTexture.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
class DummyState final : public s2d::client::ApplicationState
{
public:
    explicit DummyState(bool overlay = false)
        : m_overlay{overlay}
    {
    }

    tgui::Widget::Ptr uiRoot() const noexcept override
    {
        return root;
    }

    void update(sf::Time) override
    {
        ++updates;
        if (onUpdate)
        {
            auto callback = std::move(onUpdate);
            callback();
        }
    }

    void render(sf::RenderTarget&) override
    {
        ++renders;
    }

    void handleEvent(sf::Event const&) override
    {
        ++events;
    }

    void handleNetworkResult(s2d::client::NetworkResult const&) override
    {
        ++networkResults;
    }

    bool drawsBelow() const noexcept override
    {
        return m_overlay;
    }

    bool updatesBelow() const noexcept override
    {
        return m_overlay;
    }

    tgui::Panel::Ptr root{tgui::Panel::create()};
    int updates{};
    int renders{};
    int events{};
    int networkResults{};
    std::function<void()> onUpdate;

private:
    bool m_overlay;
};

class MinimalState final : public s2d::client::ApplicationState
{
public:
    tgui::Widget::Ptr uiRoot() const noexcept override
    {
        return root;
    }

    void update(sf::Time) override
    {
    }

    void render(sf::RenderTarget&) override
    {
    }

    tgui::Panel::Ptr root{tgui::Panel::create()};
};
} // namespace

TEST_CASE("ApplicationState provides defaults for optional behavior", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};
    s2d::client::StateManager states{
        ui,
        std::make_unique<MinimalState>()};

    states.handleEvent(sf::Event{sf::Event::Closed{}});
    states.handleNetworkResult(s2d::client::NetworkError{s2d::client::NetworkOperation::Ping, "failure"});
    states.update(sf::Time::Zero);
    states.render(target);
}

TEST_CASE("StateManager applies stack changes only when requested", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    auto initial = std::make_unique<DummyState>();
    auto* initialView = initial.get();
    s2d::client::StateManager states{ui, std::move(initial)};
    REQUIRE(initialView->root->isEnabled());

    auto pushed = std::make_unique<DummyState>();
    auto* pushedView = pushed.get();
    states.requestPush(std::move(pushed));

    states.update(sf::Time::Zero);
    REQUIRE(initialView->updates == 1);
    REQUIRE(pushedView->updates == 0);
    REQUIRE_FALSE(initialView->root->isEnabled());
    REQUIRE(pushedView->root->isEnabled());

    states.requestPop();
    states.update(sf::Time::Zero);
    REQUIRE(initialView->root->isEnabled());
}

TEST_CASE("StateManager requires and preserves at least one state", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    REQUIRE_THROWS_AS(
        (s2d::client::StateManager{ui, nullptr}),
        std::invalid_argument);

    auto initial = std::make_unique<DummyState>();
    auto* initialView = initial.get();
    s2d::client::StateManager states{ui, std::move(initial)};

    states.requestPop();
    REQUIRE_THROWS_AS(states.update(sf::Time::Zero), std::logic_error);
    REQUIRE(initialView->root->isEnabled());
}

TEST_CASE("StateManager reset replaces the entire stack", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    auto initial = std::make_unique<DummyState>();
    s2d::client::StateManager states{ui, std::move(initial)};

    states.requestPush(std::make_unique<DummyState>());
    states.update(sf::Time::Zero);

    auto replacement = std::make_unique<DummyState>();
    auto* replacementView = replacement.get();
    states.requestResetTo(std::move(replacement));
    states.update(sf::Time::Zero);
    REQUIRE(replacementView->root->isEnabled());

    states.requestPop();
    REQUIRE_THROWS_AS(states.update(sf::Time::Zero), std::logic_error);
}

TEST_CASE("StateManager routes work through a three-state overlay chain", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    auto lower = std::make_unique<DummyState>();
    auto* lowerView = lower.get();
    s2d::client::StateManager states{ui, std::move(lower)};

    auto middle = std::make_unique<DummyState>(true);
    auto* middleView = middle.get();
    states.requestPush(std::move(middle));
    states.update(sf::Time::Zero);

    auto upper = std::make_unique<DummyState>(true);
    auto* upperView = upper.get();
    states.requestPush(std::move(upper));
    states.update(sf::Time::Zero);

    lowerView->updates = 0;
    middleView->updates = 0;

    sf::Event const event{sf::Event::Closed{}};
    states.handleEvent(event);
    states.handleNetworkResult(s2d::client::NetworkError{s2d::client::NetworkOperation::Ping, "failure"});
    states.update(sf::Time::Zero);
    states.render(target);

    REQUIRE(lowerView->events == 0);
    REQUIRE(middleView->events == 0);
    REQUIRE(upperView->events == 1);
    REQUIRE(lowerView->networkResults == 1);
    REQUIRE(middleView->networkResults == 1);
    REQUIRE(upperView->networkResults == 1);
    REQUIRE(lowerView->updates == 1);
    REQUIRE(middleView->updates == 1);
    REQUIRE(upperView->updates == 1);
    REQUIRE(lowerView->renders == 1);
    REQUIRE(middleView->renders == 1);
    REQUIRE(upperView->renders == 1);
}

TEST_CASE("StateManager stops a three-state chain at a blocking state", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    auto lower = std::make_unique<DummyState>();
    auto* lowerView = lower.get();
    s2d::client::StateManager states{ui, std::move(lower)};

    auto middle = std::make_unique<DummyState>();
    auto* middleView = middle.get();
    states.requestPush(std::move(middle));
    states.update(sf::Time::Zero);

    auto upper = std::make_unique<DummyState>(true);
    auto* upperView = upper.get();
    states.requestPush(std::move(upper));
    states.update(sf::Time::Zero);

    lowerView->updates = 0;
    middleView->updates = 0;

    states.handleNetworkResult(s2d::client::NetworkError{s2d::client::NetworkOperation::Ping, "failure"});
    states.update(sf::Time::Zero);
    states.render(target);

    REQUIRE(lowerView->networkResults == 0);
    REQUIRE(middleView->networkResults == 1);
    REQUIRE(upperView->networkResults == 1);
    REQUIRE(lowerView->updates == 0);
    REQUIRE(middleView->updates == 1);
    REQUIRE(upperView->updates == 1);
    REQUIRE(lowerView->renders == 0);
    REQUIRE(middleView->renders == 1);
    REQUIRE(upperView->renders == 1);
}

TEST_CASE("StateManager applies a transition requested during update", "[client][state]")
{
    sf::RenderTexture target{{32U, 32U}};
    s2d::client::UiComponent ui{target};

    auto initial = std::make_unique<DummyState>();
    auto* initialView = initial.get();
    s2d::client::StateManager states{ui, std::move(initial)};

    auto pushed = std::make_unique<DummyState>();
    auto* pushedView = pushed.get();
    initialView->onUpdate = [&states, &pushed] {
        states.requestPush(std::move(pushed));
    };

    states.update(sf::Time::Zero);

    REQUIRE(initialView->updates == 1);
    REQUIRE(pushedView->updates == 0);
    REQUIRE_FALSE(initialView->root->isEnabled());
    REQUIRE(pushedView->root->isEnabled());

    states.update(sf::Time::Zero);
    REQUIRE(initialView->updates == 1);
    REQUIRE(pushedView->updates == 1);
}
