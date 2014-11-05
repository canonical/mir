/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/logging/always_on_console_logger.h"

namespace ml = mir::logging;

ml::AlwaysOnLogger& ml::AlwaysOnConsoleLogger::instance()
{
    static ml::AlwaysOnConsoleLogger always_on_console_logger;

    return always_on_console_logger;
}

void ml::AlwaysOnConsoleLogger::log(ml::Logger::Severity /*severity*/,
                                    const std::string& message,
                                    const std::string& component)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char now[32];

    snprintf(now, sizeof(now), "%ld.%06ld",
             (long)ts.tv_sec, ts.tv_nsec / 1000);

    std::cerr << "["
              << now
              << "] "
              << component
              << ": "
              << message
              << "\n";
}
