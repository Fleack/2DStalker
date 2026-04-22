#include "Session.hpp"

#include "logger/logger.hpp"
#include "message.pb.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <winsock2.h>

#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

namespace s2d::network
{
namespace
{
constexpr std::uint32_t kMaxMessageBytes = 1024U * 1024U;
} // namespace

Session::Session(asio::ip::tcp::socket&& socket) noexcept : m_socket{std::move(socket)}
{
}

asio::awaitable<void> Session::start()
{
    try
    {
        for (;;)
        {
            s2d::protocol::ClientMessage const request = co_await read_request();
            s2d::protocol::ServerMessage const response = dispatch(request);
            co_await write_response(response);
        }
    }
    catch (std::exception const& error)
    {
        LOG(warn, "Session finished: {}", error.what());
        stop();
    }

    co_return;
}

void Session::stop() noexcept
{
    asio::error_code shutdown_error;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, shutdown_error);

    asio::error_code close_error;
    m_socket.close(close_error);
}

asio::awaitable<s2d::protocol::ClientMessage> Session::read_request()
{
    std::uint32_t net_length = 0;
    co_await asio::async_read(m_socket, asio::buffer(&net_length, sizeof(net_length)), asio::use_awaitable);

    std::uint32_t const payload_size = ntohl(net_length);
    if (payload_size == 0 || payload_size > kMaxMessageBytes)
    {
        throw std::runtime_error("Invalid protobuf payload size");
    }

    std::string payload(payload_size, '\0');
    co_await asio::async_read(m_socket, asio::buffer(payload), asio::use_awaitable);

    s2d::protocol::ClientMessage request;
    if (!request.ParseFromArray(payload.data(), static_cast<int>(payload.size())))
    {
        throw std::runtime_error("Failed to parse ClientMessage");
    }

    co_return request;
}

asio::awaitable<void> Session::write_response(s2d::protocol::ServerMessage const& message)
{
    std::string payload;
    if (!message.SerializeToString(&payload))
    {
        throw std::runtime_error("Failed to serialize ServerMessage");
    }

    std::uint32_t const net_length = htonl(static_cast<std::uint32_t>(payload.size()));

    co_await asio::async_write(m_socket, asio::buffer(&net_length, sizeof(net_length)), asio::use_awaitable);
    co_await asio::async_write(m_socket, asio::buffer(payload), asio::use_awaitable);
    co_return;
}

s2d::protocol::ServerMessage Session::dispatch(s2d::protocol::ClientMessage const& request)
{
    s2d::protocol::ServerMessage response;
    response.set_request_id(request.request_id());

    switch (request.payload_case())
    {
    case s2d::protocol::ClientMessage::kPing:
        response.set_status(s2d::protocol::STATUS_OK);
        response.mutable_pong()->set_timestamp(request.ping().timestamp());
        break;

    case s2d::protocol::ClientMessage::kStateSnapshot:
        response.set_status(s2d::protocol::STATUS_OK);
        response.mutable_state_snapshot()->set_state_json(R"({"state":"stub"})");
        break;

    case s2d::protocol::ClientMessage::PAYLOAD_NOT_SET:
    default:
        response.set_status(s2d::protocol::STATUS_ERROR);
        response.mutable_error()->set_code(400);
        response.mutable_error()->set_message("ClientMessage payload is not set");
        break;
    }

    return response;
}

} // namespace s2d::network
