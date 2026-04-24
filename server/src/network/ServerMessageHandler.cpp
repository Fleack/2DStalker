#include "ServerMessageHandler.hpp"

#include "connection_id.hpp"
#include "logger/logger.hpp"

#include <message.pb.h>
#include <string>
#include <string_view>

namespace s2d::network
{
namespace
{
protocol::ServerMessage makeErrorResponse(int code, std::string_view message)
{
    protocol::ServerMessage response;
    response.set_status(protocol::STATUS_ERROR);
    auto* error = response.mutable_error();
    error->set_code(code);
    error->set_message(std::string{message});
    return response;
}
} // namespace

asio::awaitable<protocol::ServerMessage> ServerMessageHandler::onMessage(
    connection_id connection_id,
    protocol::ClientMessage const& message)
{
    LOG(debug, "Received from client[{}] message[id={}]", connection_id.id, message.request_id()); // TODO: improve logging

    protocol::ServerMessage response;
    switch (message.payload_case())
    {
    case protocol::ClientMessage::kPing:
        response.set_status(protocol::STATUS_OK);
        response.mutable_pong()->set_timestamp(message.ping().timestamp());
        break;
    case protocol::ClientMessage::kStateSnapshot:
        response.set_status(protocol::STATUS_OK);
        response.mutable_state_snapshot()->set_state_json(R"({"world":"bootstrap","players":[]})");
        break;
    case protocol::ClientMessage::PAYLOAD_NOT_SET:
    default:
        response = makeErrorResponse(400, "Client message payload is not set");
        break;
    }

    co_return response;
}

void ServerMessageHandler::onDisconnect(connection_id connection_id)
{
    LOG(info, "Client[{}] disconnected", connection_id.id);
}

} // namespace s2d::network
