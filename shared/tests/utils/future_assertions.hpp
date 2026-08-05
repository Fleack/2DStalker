#pragma once

#include <chrono>
#include <future>

#include <catch2/catch_test_macros.hpp>

namespace s2d::test
{

template <typename T>
void require_ready(std::future<T> const& future)
{
    REQUIRE(future.wait_for(std::chrono::seconds{0}) == std::future_status::ready);
}

} // namespace s2d::test
