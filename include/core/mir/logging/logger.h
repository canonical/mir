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
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mir
{
namespace logging
{
class Tag
{
public:
    class Impl;

    Tag(Tag const& parent, std::string_view name);
    ~Tag();

    auto name() const -> std::string_view;
private:
    friend class Impl;
    explicit Tag(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> const impl;
};

extern Tag const core;
extern Tag const input;
extern Tag const wayland;
extern Tag const graphics;
extern Tag const window_management;

using Tags = std::initializer_list<std::reference_wrapper<Tag const> const>;

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
    virtual void log(char const* component, Severity severity, char const* format, ...)
         __attribute__ ((format (printf, 4, 5)));

    virtual void log(Severity severity, Tags tags, std::string_view message);

    template<typename... Args>
    void log(Severity severity, Tags tags, std::format_string<Args...> fmt, Args&&... args)
    {
        log(severity, tags, std::format(fmt, std::forward<Args>(args)...));
    }

protected:
    Logger() {}
    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

void log(Severity severity, const std::string& message, const std::string& component);
void log(Severity severity, Tags tags, std::string_view message);
void set_logger(std::shared_ptr<Logger> const& new_logger);
void format_message(std::ostream& stream, Severity severity, std::string const& message, std::string const& component);

}
}

#endif // MIR_LOGGING_LOGGER_H_
