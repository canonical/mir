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
#include <mir/logging/logger.h>
#include <mir/logging/event.h>
#include <mir/logging/tag.h>
#include <mir/synchronised.h>
#include <mir/logging/dumb_console_logger.h>
#include <mir/fatal.h>

#include <iostream>
#include <chrono>
#include <format>
#include <mutex>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ml = mir::logging;

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

void ml::log(Severity severity, Tags tags, std::string_view fmt, std::format_args args, std::source_location location)
{
    Event const ev{severity, tags, fmt, args, location};
    if (ev.should_log())
    {
        auto const logger = get_logger();

        logger->log(ev);
    }
}

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    if (new_logger)
    {
        std::lock_guard lock{log_mutex};
        the_logger = new_logger;
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
