/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
class Logger
{
public:
    
    enum Severity
    {
        emergency = 0,
        alert = 1,
        critical = 2,
        error = 3,
        warning = 4,
        notice = 5,
        informational = 6,
        debug = 7
    };

    static const char* unknown_component_name()
    {
        static const char* name = "UnknownComponent";
        return name;
    }
    
    virtual ~Logger() {}

    template<Severity severity>
    void log(const std::string& message, const std::string& component = Logger::unknown_component_name())
    {
        log(severity, message, component);
    }
    
  protected:
    Logger() {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    virtual void log(Severity severity,
                     const std::string& message,
                     const std::string& component) = 0;
};
}
}

#endif // MIR_LOGGING_LOGGER_H_
