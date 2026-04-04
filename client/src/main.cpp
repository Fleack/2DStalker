#include "network/Client.hpp"

#include <thread>

#include <SFML/Graphics.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <nlohmann/json.hpp>

using asio::ip::tcp;

asio::io_context net_context;

void network_thread()
{
    auto work = asio::make_work_guard(net_context);
    net_context.run();
}

int main()
{
    std::thread net_thread(network_thread);

    Client client(net_context);

    // --- connect ---
    asio::post(net_context, [&client]() { co_spawn(net_context, client.connect(), asio::detached); });

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Client");
    window.setFramerateLimit(60);

    while (window.isOpen())
    {
        while (std::optional const event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (auto const* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Space)
                {
                    asio::post(net_context, [&client]() { co_spawn(net_context, client.send({{"cmd", "move"}}), asio::detached); });
                }

                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }

        window.clear(sf::Color(30, 30, 30));
        window.display();
    }

    net_context.stop();
    net_thread.join();
}
