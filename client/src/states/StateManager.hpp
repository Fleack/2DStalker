#pragma once

#include "states/ApplicationState.hpp"

#include <deque>
#include <memory>
#include <vector>

namespace s2d::client
{
class UiComponent;

class StateManager
{
public:
    StateManager(UiComponent& ui, std::unique_ptr<ApplicationState> initialState);

    void requestPush(std::unique_ptr<ApplicationState> state);
    void requestResetTo(std::unique_ptr<ApplicationState> state);
    void requestPop();

    void handleEvent(sf::Event const& event);
    void handleNetworkResult(NetworkResult const& result);
    void update(sf::Time frameTime);
    void render(sf::RenderTarget& target);

private:
    void applyPendingChanges();
    [[nodiscard]] std::size_t firstUpdatedState() const noexcept;
    [[nodiscard]] std::size_t firstRenderedState() const noexcept;
    void refreshInteractivity();

private:
    enum class ChangeKind
    {
        Push,
        ResetTo,
        Pop,
    };

    struct PendingChange
    {
        ChangeKind kind;
        std::unique_ptr<ApplicationState> state;
    };

private:
    UiComponent& m_ui;
    std::vector<std::unique_ptr<ApplicationState>> m_states;
    std::deque<PendingChange> m_pendingChanges;
};
} // namespace s2d::client
