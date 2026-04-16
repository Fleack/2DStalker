#include "logger/logger.hpp"
#include "network/Client.hpp"

#include <thread>

#include <SFML/Graphics.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_future.hpp>
#include <nlohmann/json.hpp>

using asio::ip::tcp;

int main()
{
    asio::io_context net_context;
    std::jthread worker([&] {
        auto work = asio::make_work_guard(net_context);
        net_context.run();
    });

    Client client(net_context);

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
                    nlohmann::json j = {{"cmd", "move"}};
                    auto response = co_spawn(net_context, client.send(j), asio::use_future).get();
                    LOG(info, "Response from server: {}", response);
                }

                if (key->code == sf::Keyboard::Key::C)
                {
                    asio::ip::address ip = asio::ip::make_address("127.0.0.1");
                    uint16_t port = 1234;
                    co_spawn(net_context, client.connect(ip, port), asio::use_future).get();
                }

                if (key->code == sf::Keyboard::Key::D)
                {
                    client.disconnect();
                }

                if (key->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }
            }
        }

        window.clear(sf::Color(30, 30, 30));
        window.display();
    }
}
