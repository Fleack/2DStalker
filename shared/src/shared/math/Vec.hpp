#pragma once

#include <array>
#include <cstddef>

namespace s2d::math
{
template <typename T, std::size_t N>
struct Vec
{
    static_assert(N > 0);

    using value_type = T;

    static constexpr std::size_t size() noexcept
    {
        return N;
    }

    std::array<T, N> data{};
};

template <typename T>
struct Vec2
{
    T x{};
    T y{};
};

template <typename T>
struct Vec3
{
    T x{};
    T y{};
    T z{};
};
} // namespace s2d::math
