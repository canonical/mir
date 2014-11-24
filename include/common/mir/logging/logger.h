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

#ifndef MIR_LOGGING_LOGGER_H_
#define MIR_LOGGING_LOGGER_H_

#include <memory>
#include <string>

#ifndef MIR_LOGGING_COMPONENT
#ifndef MIR_LOGGING_COMPONENT_FALLBACK
#define MIR_LOGGING_COMPONENT_FALLBACK "?"
#endif
#define MIR_LOGGING_COMPONENT MIR_LOGGING_COMPONENT_FALLBACK
#endif

namespace mir
{
namespace logging
{

enum class Severity
{
    critical = 0,
    error = 1,
    warning = 2,
    informational = 3,
    debug = 4
};

// A facade to shield the inner core of mir to prevent an actual
// logging framework from leaking implementation detail.
class Logger
{
public:
    virtual void log(Severity severity,
                     const std::string& message,
                     const std::string& component = MIR_LOGGING_COMPONENT) = 0;

protected:
    Logger() {}
    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

void log(Severity severity, const std::string& message, const std::string& component = MIR_LOGGING_COMPONENT);
void set_logger(std::shared_ptr<Logger> const& new_logger);

}
}

#endif // MIR_LOGGING_LOGGER_H_
