#include <asio.hpp>
#include <thread>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using asio::ip::tcp;

asio::io_context net_context;

class Client
{
public:
    explicit Client(asio::io_context& ctx)
        : socket_(ctx) {}

    asio::awaitable<void> connect()
    {
        auto executor = co_await asio::this_coro::executor;

        tcp::resolver resolver(executor);
        auto endpoints = co_await resolver.async_resolve("127.0.0.1", "1234", asio::use_awaitable);

        co_await asio::async_connect(socket_, endpoints, asio::use_awaitable);

        spdlog::info("Connected to server");
    }

    asio::awaitable<void> send(nlohmann::json j)
    {
        std::string data = j.dump();
        uint32_t len = htonl(static_cast<uint32_t>(data.size()));

        co_await asio::async_write(socket_, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
        co_await asio::async_write(socket_, asio::buffer(data), asio::use_awaitable);

        // --- read response ---
        uint32_t net_len;
        co_await asio::async_read(socket_, asio::buffer(&net_len, sizeof(net_len)), asio::use_awaitable);

        uint32_t resp_len = ntohl(net_len);

        std::string resp(resp_len, '\0');
        co_await asio::async_read(socket_, asio::buffer(resp), asio::use_awaitable);

        spdlog::info("Server response: {}", resp);
    }

private:
    tcp::socket socket_;
};

void network_thread()
{
    auto work = asio::make_work_guard(net_context);
    net_context.run();
}

int main()
{
    spdlog::set_level(spdlog::level::info);

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
