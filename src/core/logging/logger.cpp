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

#include <mir/logging/logger.h>

#include <mir/synchronised.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/logger.h>
#include <boost/throw_exception.hpp>
#include <concepts>
#include <mir/logging/logger.h>
#include <core/logging_internal.h>

#include <mir/synchronised.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/fatal.h>

#include <atomic>
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
#include <string>
#include <string_view>
#include <list>

namespace ml = mir::logging;

struct ml::Tag
{
    std::string const name;
    std::atomic<Severity> logging_severity;
    Tag const* parent;
};

namespace
{
auto construct_initial_tag_list() -> std::list<ml::Tag>
{
    decltype(construct_initial_tag_list()) list;
    list.emplace_back("base", ml::Severity::warning, nullptr);
    return list;
}
}

mir::Synchronised<std::list<ml::Tag>> known_tags{construct_initial_tag_list()}; //TICS !cppcoreguidelines-avoid-non-const-global-variables - This is the list of tags, shared within this module

auto ml::create_tag(Tag const& parent, std::string_view name) -> Tag const&
{
    auto locked_tags = known_tags.lock();
    locked_tags->emplace_back(std::string{name}, Severity::warning, &parent);
    return locked_tags->back();
}

auto ml::base() -> Tag const&
{
    static Tag const& base = known_tags.lock()->front();
    return base;
}

auto ml::input() -> Tag const&
{
    static Tag const& input = create_tag(base(), "input");
    return input;
}

auto ml::wayland() -> Tag const&
{
    static Tag const& wayland = create_tag(base(), "wayland");
    return wayland;
}

auto ml::graphics() -> Tag const&
{
    static Tag const& graphics = create_tag(base(), "graphics");
    return graphics;
}

auto ml::window_management() -> Tag const&
{
    static Tag const& window_management = create_tag(base(), "window-management");
    return window_management;
}

void ml::Logger::log(char const* component, Severity severity, char const* format, ...)
{
    auto const bufsize = 4096;
    va_list va;
    va_start(va, format);
    char message[bufsize];
    std::vsnprintf(message, bufsize, format, va);
    va_end(va);

    // Inefficient, but maintains API: Constructing a std::string for message/component.
    log(severity, std::string{message}, std::string{component});
}

void ml::Logger::log(Severity severity, Tags tags, std::string_view message)
{
    // Default implementation uses :-delimited tags as the component
    // TODO: Remove the log(Severity, std::string, std::string) interface and replace
    // with this one.

    for (auto const& tag: tags)
    {
        if (logging_enabled_for(tag, severity))
        {
            std::string component;
            std::ranges::copy(
                tags | std::views::transform([](Tag const& tag) { return tag.name; }) | std::views::join_with(':'),
                std::back_inserter(component));
            log(severity, std::string{message}, component);
            return;
        }
    }
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

namespace
{
auto split_assignment(std::string const& value)
    -> std::pair<std::string_view, std::string_view>
{
    auto equals_pos = value.find('=');
    if (equals_pos == value.length() || equals_pos == value.npos)
    {
      BOOST_THROW_EXCEPTION((std::runtime_error{
          std::format("Failed to parse log-level: {}", value)}));
    }
    return std::make_pair(
        std::string_view{value.data(), equals_pos},
        std::string_view{value.data() + equals_pos + 1, value.length() - equals_pos - 1});
}

auto lookup_tag(decltype(known_tags)::Locked const& tag_list, std::string_view tag_name) -> ml::Tag&
{
    for (auto& tag : *tag_list)
    {
        if (tag.name == tag_name)
        {
            return tag;
        }
    }
    BOOST_THROW_EXCEPTION((std::out_of_range{std::format("Looking up nonexistent tag: {}", tag_name)}));
}

auto parse_severity(std::string_view const severity_name) -> ml::Severity
{
    if (severity_name == "critical")
    {
        return ml::Severity::critical;
    }
    else if (severity_name == "error")
    {
        return ml::Severity::error;
    }
    else if (severity_name == "warn" || severity_name == "warning")
    {
        return ml::Severity::warning;
    }
    else if (severity_name == "info" || severity_name == "informational")
    {
        return ml::Severity::informational;
    }
    else if (severity_name == "debug")
    {
        return ml::Severity::debug;

    }
    BOOST_THROW_EXCEPTION((std::out_of_range{std::format("Unrecognised severity: {}", severity_name)}));
}

template<typename Functor>
requires std::invocable<Functor, ml::Tag&>
void for_each_child(ml::Tag const& parent, decltype(known_tags)::Locked const& tag_list, Functor&& func)
{
    for (auto& tag : *tag_list)
    {
        if (tag.parent == &parent)
        {
            func(tag);
            for_each_child(tag, tag_list, std::forward<Functor&&>(func));
        }
    }
}

void update_tag_filtering(std::vector<std::string> const& option_values)
{
    auto locked_tags = known_tags.lock();
    for (auto const& value : option_values)
    {
        try
        {
            auto [tag_name, severity_name] = split_assignment(value);
            auto& tag = lookup_tag(locked_tags, tag_name);
            auto const severity = parse_severity(severity_name);
            tag.logging_severity = severity;
            for_each_child(tag, locked_tags, [severity](ml::Tag& tag) { tag.logging_severity = severity; });
        }
        catch(std::exception const& err)
        {
            ml::log(ml::Severity::critical, {ml::base()}, std::format("Failed to parse log-level option “{}”: {}", value, err.what()));
        }
    }
}

auto full_path_to_tag(ml::Tag const& tag) -> std::string
{
    if (!tag.parent)
    {
        return tag.name;
    }
    else
    {
        return full_path_to_tag(*tag.parent) + "/" + tag.name;
    }
}
}

auto ml::logging_enabled_for(const Tag &tag, Severity sev) -> bool
{
    return tag.logging_severity >= sev;
}

void ml::add_logging_options(
    boost::program_options::options_description_easy_init options)
{
    namespace po = boost::program_options;

    // Ensure the tags are registered.
    base();
    input();
    graphics();
    wayland();
    window_management();

    std::string tag_descriptions;
    std::ranges::copy(
        *known_tags.lock() |
            std::views::transform([](Tag const& tag) { return full_path_to_tag(tag); }) | std::views::join_with('\n'),
        std::back_inserter(tag_descriptions));


    options("log-level",
        po::value<std::vector<std::string>>()->notifier(update_tag_filtering),
        ("Minimum severity of a log message required for it to be printed.\n"
        "Valid severities are: critical, error, warning, informational, debug (“warn” and “info” can be used as short forms of “warning” and “informational”)\n"
        "Must be specified in the form “tag=severity”\n"
        "Possible tags to filter on are:\n"
        "\t" + tag_descriptions).c_str());
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

// GCC and Clang both ensure the switch is exhaustive.
// GCC, however, gets a "control reaches end of non-void function" warning without this
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

auto std::formatter<ml::Severity>::format(ml::Severity sev, std::format_context& ctx) const
    -> std::format_context::iterator
{
    auto const sev_to_str = [](ml::Severity sev)
        {
            switch (sev)
            {
                case ml::Severity::critical:
                    return "critical";
                case ml::Severity::error:
                    return "error";
                case ml::Severity::warning:
                    return "warning";
                case ml::Severity::informational:
                    return "info";
                case ml::Severity::debug:
                    return "debug";
            }
        };
    return std::formatter<std::string>::format(sev_to_str(sev), ctx);
}

#ifndef __clang__
#pragma GCC diagnostic pop
#endif
