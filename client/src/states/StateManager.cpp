#include "states/StateManager.hpp"

#include "ui/UiComponent.hpp"

#include <stdexcept>
#include <utility>

namespace s2d::client
{
StateManager::StateManager(UiComponent& ui, std::unique_ptr<ApplicationState> initialState)
    : m_ui{ui}
{
    if (!initialState)
    {
        throw std::invalid_argument{"Initial application state must not be null"};
    }

    m_ui.mount(initialState->uiRoot());
    m_states.push_back(std::move(initialState));
    refreshInteractivity();
}

void StateManager::requestPush(std::unique_ptr<ApplicationState> state)
{
    if (!state)
    {
        throw std::invalid_argument{"Pushed application state must not be null"};
    }
    m_pendingChanges.push_back(PendingChange{ChangeKind::Push, std::move(state)});
}

void StateManager::requestResetTo(std::unique_ptr<ApplicationState> state)
{
    if (!state)
    {
        throw std::invalid_argument{"Reset application state must not be null"};
    }
    m_pendingChanges.push_back(PendingChange{ChangeKind::ResetTo, std::move(state)});
}

void StateManager::requestPop()
{
    m_pendingChanges.push_back(PendingChange{ChangeKind::Pop, nullptr});
}

void StateManager::handleEvent(sf::Event const& event)
{
    m_states.back()->handleEvent(event);
}

void StateManager::handleNetworkResult(NetworkResult const& result)
{
    for (auto index = firstUpdatedState(); index < m_states.size(); ++index)
    {
        m_states[index]->handleNetworkResult(result);
    }
}

void StateManager::update(sf::Time frameTime)
{
    for (auto index = firstUpdatedState(); index < m_states.size(); ++index)
    {
        m_states[index]->update(frameTime);
    }

    applyPendingChanges();
}

void StateManager::render(sf::RenderTarget& target)
{
    for (auto index = firstRenderedState(); index < m_states.size(); ++index)
    {
        m_states[index]->render(target);
    }
}

void StateManager::applyPendingChanges()
{
    if (m_pendingChanges.empty())
    {
        return;
    }

    while (!m_pendingChanges.empty())
    {
        auto change = std::move(m_pendingChanges.front());
        m_pendingChanges.pop_front();

        switch (change.kind)
        {
        case ChangeKind::Push:
            m_ui.mount(change.state->uiRoot());
            m_states.push_back(std::move(change.state));
            break;
        case ChangeKind::ResetTo:
            m_ui.mount(change.state->uiRoot());
            for (auto const& state : m_states)
            {
                m_ui.unmount(state->uiRoot());
            }
            m_states.clear();
            m_states.push_back(std::move(change.state));
            break;
        case ChangeKind::Pop:
            if (m_states.size() == 1)
            {
                throw std::logic_error{"Cannot pop the last application state"};
            }
            m_ui.unmount(m_states.back()->uiRoot());
            m_states.pop_back();
            break;
        }
    }

    refreshInteractivity();
}

std::size_t StateManager::firstUpdatedState() const noexcept
{
    auto first = m_states.size() - 1;
    while (first > 0 && m_states[first]->updatesBelow())
    {
        --first;
    }
    return first;
}

std::size_t StateManager::firstRenderedState() const noexcept
{
    auto first = m_states.size() - 1;
    while (first > 0 && m_states[first]->drawsBelow())
    {
        --first;
    }
    return first;
}

void StateManager::refreshInteractivity()
{
    for (auto const& state : m_states)
    {
        m_ui.setInteractive(state->uiRoot(), state.get() == m_states.back().get());
    }
}
} // namespace s2d::client
