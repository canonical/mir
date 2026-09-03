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

#include <mir/logging/tag.h>

#include <format>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mir
{
namespace logging
{
// A facade to shield the inner core of mir to prevent an actual
// logging framework from leaking implementation detail.
class Logger
{
public:
    virtual void log(Severity severity, std::string const& message, std::string const& component) = 0;

    /*
     * Those playing at home may wonder why we're saying the 4th argument is the format string,
     * when it's the 3rd argument in the signature.
     *
     * The answer, of course, is that the attribute doesn't know about the implicit
     * 'this' first parameter of C++!
     */
    virtual void log(char const* component, Severity severity, char const* format, ...)
        __attribute__((format(printf, 4, 5)));

    /*
     * Cruft removal: when we're sure we've got the right API/ABI, we should
     * *remove* log(severity, string, string) above and make this the
     * required-to-implement interface.
     */
    void log(Severity severity, Tags tags, std::string_view message);

    template<typename... Args>
    void log(Severity severity, Tags tags, std::format_string<Args...> fmt, Args&&... args)
    { log(severity, tags, std::format(fmt, std::forward<Args>(args)...)); }

protected:

    Logger() {}
    virtual ~Logger() = default;
    Logger(Logger const&) = delete;
    Logger& operator=(Logger const&) = delete;
};

void log(Severity severity, std::string const& message, std::string const& component);
void log(Severity severity, Tags tags, std::string_view message);
void set_logger(std::shared_ptr<Logger> const& new_logger);
void format_message(std::ostream& stream, Severity severity, std::string const& message, std::string const& component);

}
}

#endif // MIR_LOGGING_LOGGER_H_
