#include "network/Client.hpp"
#include "shared/logger/logger.hpp"
#include "shared/protocol/message.pb.h"

#include <thread>

#include <SFML/Graphics.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_future.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/fmt/bin_to_hex.h>

using asio::ip::tcp;

int main()
{
    asio::io_context net_context;
    std::jthread worker([&] {
        auto work = asio::make_work_guard(net_context);
        net_context.run();
    });

    auto client = Client::create(net_context);

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
                    s2d::protocol::ClientMessage message;
                    message.mutable_state_snapshot();
                    auto response = co_spawn(net_context, client->send(message), asio::use_future).get();
                    LOG(info, "Response from server: {}", response.SerializeAsString());
                }

                if (key->code == sf::Keyboard::Key::P)
                {
                    s2d::protocol::ClientMessage message;

                    auto nowMs = duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();
                    message.mutable_ping()->set_timestamp(static_cast<uint64_t>(nowMs));
                    try
                    {
                        auto response = co_spawn(net_context, client->send(message), asio::use_future).get();
                        LOG(info, "Response from server: {:np}", spdlog::to_hex(response.SerializeAsString()));
                    }
                    catch (std::exception const& ex)
                    {
                        LOG(err, "Failed to send ping: {}", ex.what());
                    }
                }

                if (key->code == sf::Keyboard::Key::C)
                {
                    if (client->isConnected())
                    {
                        LOG(warn, "Client is already connected");
                        continue;
                    }
                    asio::ip::address ip = asio::ip::make_address("127.0.0.1");
                    uint16_t port = 1234;
                    co_spawn(net_context, client->connect(ip, port), asio::use_future).get();
                }

                if (key->code == sf::Keyboard::Key::D)
                {
                    client->disconnect();
                }

                if (key->code == sf::Keyboard::Key::Escape)
                {
                    if (client->isConnected())
                    {
                        client->disconnect();
                    }
                    window.close();
                }
            }
        }

        window.clear(sf::Color(30, 30, 30));
        window.display();
    }
    net_context.stop();
}
