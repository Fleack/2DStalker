#pragma once

#include "network/NetworkComponent.hpp"
#include "render/RenderComponent.hpp"
#include "states/StateManager.hpp"
#include "ui/UiComponent.hpp"

namespace s2d::client
{
class Application
{
public:
    Application();

    void run();

private:
    RenderComponent m_render;
    NetworkComponent m_network;
    UiComponent m_ui;
    StateManager m_states;
};
} // namespace s2d::client
