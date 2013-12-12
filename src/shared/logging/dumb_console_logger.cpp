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
#include <chrono>
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

    using namespace std::chrono;
    unsigned long long microsec = duration_cast<microseconds>(
        system_clock::now().time_since_epoch()
        ).count();
    char now[32];
    snprintf(now, sizeof(now), "%llu.%06llu",
             microsec / 1000000ULL, microsec % 1000000ULL);

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
