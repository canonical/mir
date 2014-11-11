/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "mir/logging/dumb_console_logger.h"

#include <mutex>
#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;

void ml::DumbConsoleLogger::log(ml::Logger::Severity severity,
                                const std::string& message,
                                const std::string& component)
{

    static const char* lut[5] =
            {
                "CC",
                "EE",
                "WW",
                "II",
                "DD"
            };

    std::ostream& out = severity < Logger::informational ? std::cerr : std::cout;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char now[32];
    snprintf(now, sizeof(now), "%ld.%06ld",
             (long)ts.tv_sec, ts.tv_nsec / 1000);

    out << "["
        << now
        << "] ("
        << lut[severity]
        << ") "
        << component
        << ": "
        << message
        << "\n";
}

namespace
{
std::mutex log_mutex;
std::shared_ptr<ml::Logger> the_logger;

std::shared_ptr<ml::Logger> get_logger()
{
    if (auto const result = the_logger)
    {
        return result;
    }
    else
    {
        std::lock_guard<decltype(log_mutex)> lock{log_mutex};
        if (!the_logger)
            the_logger = std::make_shared<ml::DumbConsoleLogger>();

        return the_logger;
    }
}
}

void ml::log(Logger::Severity severity, const std::string& message)
{
    log(severity, message, "UnknownComponent");
}

void ml::log(Logger::Severity severity,const std::string& message, const std::string& component)
{
    auto const logger = get_logger();

    logger->log(severity, message, component);
}

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    std::lock_guard<decltype(log_mutex)> lock{log_mutex};
    if (!new_logger)
        the_logger = new_logger;
}
