#include "Application.hpp"

#include "states/ClientConsoleState.hpp"

#include <memory>
#include <optional>

#include <boost/asio/ip/address.hpp>

namespace s2d::client
{
Application::Application()
    : m_render{sf::VideoMode{{1000U, 680U}}, "TextStalker Client", 60U}
    , m_network{}
    , m_ui{m_render.window()}
    , m_states{
          m_ui,
          std::make_unique<ClientConsoleState>(
              m_network,
              boost::asio::ip::tcp::endpoint{
                  boost::asio::ip::make_address("127.0.0.1"),
                  1234},
              [this] {
                  m_render.close();
              })}
{
}

void Application::run()
{
    while (m_render.isOpen())
    {
        auto const frameTime = m_render.nextFrameTime();

        while (std::optional event = m_render.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                m_render.close();
                continue;
            }

            if (!m_ui.handleEvent(*event))
            {
                m_states.handleEvent(*event);
            }
        }

        if (!m_render.isOpen())
        {
            break;
        }

        for (auto const& networkResult : m_network.drainResults())
        {
            m_states.handleNetworkResult(networkResult);
        }

        m_states.update(frameTime);

        m_render.beginFrame();
        m_states.render(m_render.target());
        m_ui.draw();
        m_render.present();
    }
}
} // namespace s2d::client
