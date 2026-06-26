#pragma once

#include "shared/protocol/message.pb.h"

#include <cstdint>

namespace s2d::test::network
{

inline protocol::ClientMessage make_ping_request(
    std::uint64_t request_id,
    std::uint64_t timestamp = 1234)
{
    protocol::ClientMessage request;
    request.set_request_id(request_id);
    request.mutable_ping()->set_timestamp(timestamp);
    return request;
}

inline protocol::ServerMessage make_pong_response(
    std::uint64_t request_id,
    std::uint64_t timestamp = 1234)
{
    protocol::ServerMessage response;
    response.set_request_id(request_id);
    response.set_status(protocol::STATUS_OK);
    response.mutable_pong()->set_timestamp(timestamp);
    return response;
}

} // namespace s2d::test::network
