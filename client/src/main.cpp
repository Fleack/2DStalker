#include "network/Client.hpp"
#include "shared/logger/logger.hpp"

#include <thread>

#include <SFML/Graphics.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_future.hpp>
#include <nlohmann/json.hpp>

using asio::ip::tcp;

namespace
{
uint64_t nextRequestId()
{
    static std::atomic<uint64_t> id{0};
    return ++id;
}
} // namespace

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
                    s2d::protocol::ClientMessage message;
                    message.set_request_id(nextRequestId());
                    message.mutable_state_snapshot();
                    auto response = co_spawn(net_context, client.send(message), asio::use_future).get();
                    LOG(info, "Response from server: {}", response.SerializeAsString());
                }

                if (key->code == sf::Keyboard::Key::P)
                {
                    s2d::protocol::ClientMessage message;
                    message.set_request_id(nextRequestId());

                    auto nowMs = duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();
                    message.mutable_ping()->set_timestamp(static_cast<uint64_t>(nowMs));
                    auto response = co_spawn(net_context, client.send(message), asio::use_future).get();
                    LOG(info, "Response from server: {}", response.SerializeAsString());
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
    net_context.stop();
}
