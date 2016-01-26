/*
 * Copyright © 2012 Canonical Ltd.
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

#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;

void ml::DumbConsoleLogger::log(ml::Severity severity,
                                const std::string& message,
                                const std::string& component)
{

    static const char* lut[5] =
    {
        "<CRITICAL> ",
        "<ERROR> ",
        "<WARNING> ",
        "",
        "<DEBUG> "
    };

    std::ostream& out = severity < ml::Severity::informational ? std::cerr : std::cout;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char datetime[32];
    char now[32];
    strftime(datetime, sizeof(datetime), "%F %T", localtime(&ts.tv_sec));
    snprintf(now, sizeof(now), "%s.%06ld", datetime, ts.tv_nsec / 1000);

    out << "["
        << now
        << "] "
        << lut[static_cast<int>(severity)]
        << component
        << ": "
        << message
        << std::endl;
}
