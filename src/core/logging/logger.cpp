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

#include <mir/fatal.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/logger.h>

#include <iostream>
#include <chrono>
#include <format>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <stdexcept>

namespace ml = mir::logging;

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

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    if (new_logger)
    {
        std::lock_guard lock{log_mutex};
        the_logger = new_logger;
    }
}

namespace {

using OutIter = std::ostreambuf_iterator<char>;

OutIter format_timestamp(OutIter iter)
{
    auto now = std::chrono::system_clock::now();
    auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    try {
        auto local =
            std::chrono::zoned_time{std::chrono::current_zone(), now_us};
        iter = std::format_to(iter, "[{:%F %T}]", local);
    } catch (std::runtime_error const& e) {
        mir::fatal_error_abort("Cannot format timestamp: %s", e.what());
    }
    return iter;
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

    OutIter iter{out};
    iter = format_timestamp(iter);
    format_to(iter, "{}{}: {}\n",
        lut[static_cast<int>(severity)], component, message);
    out.flush();
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
