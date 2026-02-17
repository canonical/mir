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

#ifndef MIR_LOGGING_LOGGER_H_
#define MIR_LOGGING_LOGGER_H_

#include <format>
#include <memory>
#include <string>
#include <iosfwd>

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
                     const std::string& component) = 0;

    template <typename... Args>
    void log(char const* component, Severity severity, std::format_string<Args...> fmt, Args&&... args)
    {
        log(severity, std::format(fmt, std::forward<Args>(args)...), component);
    }

protected:
    Logger() {}
    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

void log(Severity severity, const std::string& message, const std::string& component);
void set_logger(std::shared_ptr<Logger> const& new_logger);
void format_message(std::ostream& stream, Severity severity, std::string const& message, std::string const& component);

}
}

#endif // MIR_LOGGING_LOGGER_H_
