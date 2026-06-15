#pragma once

#include <cstdint>
#include <functional>

namespace s2d::network
{
struct connection_id
{
    std::uint64_t id;

    auto operator<=>(connection_id const& lhs) const noexcept = default;

    connection_id& operator++() noexcept
    {
        ++id;
        return *this;
    }

    connection_id operator++(int) noexcept
    {
        connection_id const copy = *this;
        ++id;
        return copy;
    }
};
} // namespace s2d::network

template <>
struct std::hash<s2d::network::connection_id>
{
    std::size_t operator()(s2d::network::connection_id const& v) const noexcept
    {
        return std::hash<uint64_t>{}(v.id);
    }
};
