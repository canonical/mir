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

#include <atomic>
#include <mir/logging/logger.h>

#include <mir/synchronised.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/logger.h>
#include <boost/throw_exception.hpp>
#include <mir/logging/logger.h>
#include <mir/logging/event.h>
#include <mir/logging/tag.h>
#include <mir/synchronised.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/fatal.h>

#include <iostream>
#include <chrono>
#include <format>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ml = mir::logging;

void ml::Logger::log(char const* component, Severity severity, char const* format, ...)
{
    auto const bufsize = 4096;
    va_list va;
    va_start(va, format);
    char message[bufsize];
    std::vsnprintf(message, bufsize, format, va);
    va_end(va);

    Event const ev{severity, std::string{component}, std::string{message}};
    if (ev.should_log())
    {
        log(ev);
    }
}

namespace
{
std::atomic<std::shared_ptr<ml::Logger>> the_logger{std::make_shared<mir::logging::DumbConsoleLogger>()};
}

void ml::log(ml::Severity severity, const std::string& message, const std::string& component, std::source_location location)
{
    Event const ev{severity, component, message, location};

    if (ev.should_log())
    {
        the_logger.load(std::memory_order_acquire)->log(ev);
    }
}

void ml::log(Severity severity, Tags tags, std::string_view message, std::source_location location)
{
    Event const ev{severity, tags, message, location};
    if (ev.should_log())
    {
        the_logger.load(std::memory_order_acquire)->log(ev);
    }
}

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    if (new_logger)
    {
        the_logger.store(new_logger, std::memory_order_release);
    }
}

void ml::format_message(std::ostream& out, Severity severity, std::string_view message, std::string_view component)
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
