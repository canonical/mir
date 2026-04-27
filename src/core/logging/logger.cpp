/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/logger.h>
#include <mir/fatal.h>

#include <iostream>
#include <chrono>
#include <format>
#include <mutex>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string_view>

namespace ml = mir::logging;

class ml::Tag::Impl
{
public:
    Impl(Tag const& parent, std::string_view name)
        : name_{name},
          parent{&parent}
    {
    }

    auto name() const -> std::string_view
    {
        return name_;
    }

    static auto make_top_level_tag(std::string_view name) -> Tag
    {
        return Tag{std::unique_ptr<Impl>(new Impl{name})};
    }
private:
    Impl(std::string_view name)
        : name_{name},
          parent{nullptr}
    {
    }

    std::string name_;
    [[maybe_unused]]
    Tag const* parent;
};

ml::Tag::Tag(Tag const& parent, std::string_view name)
    : impl{std::make_unique<Impl>(parent, name)}
{
}

ml::Tag::~Tag() = default;

ml::Tag::Tag(std::unique_ptr<Impl> impl)
    : impl{std::move(impl)}
{
}

auto ml::Tag::name() const -> std::string_view
{
    return impl->name();
}

ml::Tag const ml::core{ml::Tag::Impl::make_top_level_tag("core")};
ml::Tag const ml::input{ml::Tag::Impl::make_top_level_tag("input")};
ml::Tag const ml::wayland{ml::Tag::Impl::make_top_level_tag("wayland")};
ml::Tag const ml::graphics{ml::Tag::Impl::make_top_level_tag("graphics")};
ml::Tag const ml::window_management{ml::Tag::Impl::make_top_level_tag("window_management")};

void ml::Logger::log(char const* component, Severity severity, char const* format, ...)
{
    auto const bufsize = 4096;
    va_list va;
    va_start(va, format);
    char message[bufsize];
    vsnprintf(message, bufsize, format, va);
    va_end(va);

    // Inefficient, but maintains API: Constructing a std::string for message/component.
    log(severity, std::string{message}, std::string{component});
}

void ml::Logger::log(Severity severity, Tags tags, std::string_view message)
{
    // Default implementation uses :-delimited tags as the component
    // TODO: Remove the log(Severity, std::string, std::string) interface and replace
    // with this one.


    std::string component;
    std::ranges::copy(
        tags | std::views::transform([](Tag const& tag) { return tag.name(); }) | std::views::join_with(':'),
        std::back_inserter(component));
    log(severity, std::string{message}, component);
}

namespace
{
std::mutex log_mutex;
std::shared_ptr<ml::Logger> the_logger;

std::shared_ptr<ml::Logger> get_logger()
{
    std::lock_guard lock{log_mutex};

    if (!the_logger)
        the_logger = std::make_shared<ml::DumbConsoleLogger>();

    return the_logger;
}
}

void ml::log(ml::Severity severity, const std::string& message, const std::string& component)
{
    auto const logger = get_logger();

    logger->log(severity, message, component);
}

void ml::log(Severity severity, Tags tags, std::string_view message)
{
    auto const logger = get_logger();

    logger->log(severity, tags, message);
}

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    if (new_logger)
    {
        std::lock_guard lock{log_mutex};
        the_logger = new_logger;
    }
}

void ml::format_message(std::ostream& out, Severity severity, std::string const& message, std::string const& component)
{
    static const char* lut[5] =
    {
        "< CRITICAL! > ",
        "< - ERROR - > ",
        "< -warning- > ",
        "<information> ",
        "< - debug - > "
    };

    if (!out || !out.good())
    {
        std::cerr << "Failed to write to log file: " << errno << std::endl;
        return;
    }

    try
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto now_us = time_point_cast<microseconds>(now);
        auto local = zoned_time{current_zone(), now_us};
        std::format_to(std::ostreambuf_iterator{out}, "[{:%F %T}] {}{}: {}\n",
            local, lut[static_cast<int>(severity)], component, message);
        out.flush();
    }
    catch (std::runtime_error const& e)
    {
        mir::fatal_error_abort("Cannot format log message: %s", e.what());
    }
}

namespace mir
{
namespace logging
{
// For backwards compatibility (avoid breaking ABI)
void log(ml::Severity severity, std::string const& message)
{
    ml::log(severity, message, "");
}
}
}
