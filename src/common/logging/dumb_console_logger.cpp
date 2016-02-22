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

#include <unistd.h>
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

    int fd = severity < ml::Severity::informational ? STDERR_FILENO
                                                    : STDOUT_FILENO; 
    char const* env = getenv("MIR_LOG_FD");
    if (env)
        fd = atoi(env);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char now[32];
    auto offset = strftime(now, sizeof(now), "%F %T", localtime(&ts.tv_sec));
    snprintf(now+offset, sizeof(now)-offset, ".%06ld", ts.tv_nsec / 1000);

    auto line = std::string{"["} + now + "] " +
                lut[static_cast<int>(severity)] + component + ": " +
                message + "\n";
    (void)write(fd, line.data(), line.size());
}
