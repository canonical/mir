/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MIR_LOGGING_LOGGER_H_
#define MIR_LOGGING_LOGGER_H_

#include <string>

namespace mir
{
namespace logging
{
// A facade to shield the inner core of mir to prevent an actual
// logging framework from leaking implementation detail.
class Logger
{
public:
    enum Severity
    {
        critical = 0,
        error = 1,
        warning = 2,
        informational = 3,
        debug = 4
    };

    template<Severity severity>
    void log(const std::string& message)
    {
        static const std::string unknown_component("UnknownComponent");
        log(severity, message, unknown_component);
    }

    template<Severity severity>
    void log(const std::string& message, const std::string& component)
    {
        log(severity, message, component);
    }

protected:
    Logger() {}
    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    virtual void log(Severity severity,
                     const std::string& message,
                     const std::string& component) = 0;
};
}
}

#endif // MIR_LOGGING_LOGGER_H_
