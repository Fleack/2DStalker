#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include <fmt/format.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace s2d::logger::detail
{
consteval char const* short_file_impl(char const* path, std::size_t size) noexcept
{
    char const* result = path;
    for (std::size_t i = 0; i < size; ++i)
    {
        if (path[i] == '/' || path[i] == '\\')
        {
            result = path + i + 1;
        }
    }
    return result;
}

template <std::size_t N>
consteval char const* short_file(char const (&path)[N]) noexcept
{
    return short_file_impl(path, N);
}

inline std::shared_ptr<spdlog::logger>& default_logger() noexcept
{
    static auto logger = [] {
        auto lg = spdlog::stdout_color_mt("app");

        // [%^...%$] — color
        // %v        — user message
        // %s        — short filename
        // %#        — line
        // %!        — function
        lg->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v [%s:%# %!()]");
        lg->set_level(spdlog::level::trace);
        lg->flush_on(spdlog::level::err);

        return lg;
    }();

    return logger;
}

template <typename... Args>
void log(
    spdlog::logger& logger,
    spdlog::level::level_enum level,
    spdlog::source_loc loc,
    fmt::format_string<Args...> fmt_str,
    Args&&... args)
{
    logger.log(loc, level, fmt_str, std::forward<Args>(args)...);
}

} // namespace s2d::logger::detail

#define LOG(LogLevel, ...)                                                                \
    ::s2d::logger::detail::log(                                                                \
        *::s2d::logger::detail::default_logger(),                                              \
        ::spdlog::level::LogLevel,                                                        \
        ::spdlog::source_loc{::s2d::logger::detail::short_file(__FILE__), __LINE__, __func__}, \
        __VA_ARGS__)
