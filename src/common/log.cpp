/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/log.h"
#include "mir/logging/logger.h"
#include <cstdio>

#include <exception>
#include <boost/exception/diagnostic_information.hpp>

namespace mir {

void logv(logging::Severity sev, char const* component,
          char const* fmt, va_list va)
{
    char message[1024];
    int max = sizeof(message) - 1;
    int len = vsnprintf(message, max, fmt, va);
    if (len > max)
        len = max;
    message[len] = '\0';

    // Suboptimal: Constructing a std::string for message/component.
    logging::log(sev, message, component);
}

void log(logging::Severity sev, char const* component,
         char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    logv(sev, component, fmt, va);
    va_end(va);
}

void log(logging::Severity sev, char const* component,
         std::string const& message)
{
    logging::log(sev, message, component);
}


void log(
    logging::Severity severity,
    char const* component,
    std::exception_ptr const& ex,
    std::string const& message)
{
    try
    {
        std::rethrow_exception(ex);
    }
    catch(std::exception const& err)
    {
        // TODO: We can probably format this better by pulling out
        // the boost::errinfo's ourselves.
        mir::log(
            severity,
            component,
            "%s: %s",
            message.c_str(),
            boost::diagnostic_information(err).c_str());
    }
    catch(...)
    {
        mir::log(
            severity,
            component,
            "%s: unknown exception",
            message.c_str());
    }
}

} // namespace mir
