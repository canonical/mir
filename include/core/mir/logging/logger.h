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

    /*
     * Those playing at home may wonder why we're saying the 4th argument is the format string,
     * when it's the 3rd argument in the signature.
     *
     * The answer, of course, is that the attribute doesn't know about the implicit
     * 'this' first parameter of C++!
     */
    [[deprecated("Use the std::format-based overload instead")]]
    virtual void log(char const* component, Severity severity, char const* format, ...)
         __attribute__ ((format (printf, 4, 5)));

    // This template overload uses std::format syntax (brace-based) instead of printf syntax
    // Explicit overload for the no-arguments case to avoid ambiguity with variadic version
    void log(char const* component, Severity severity, std::format_string<> fmt)
    {
        log(severity, std::format(fmt), std::string{component});
    }

    // Template overload for cases with format arguments
    template <typename... Args>
    requires (sizeof...(Args) > 0)
    void log(char const* component, Severity severity, std::format_string<Args...> fmt, Args&&... args)
    {
        // TODO can this be moved to the source file for API stability?
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
