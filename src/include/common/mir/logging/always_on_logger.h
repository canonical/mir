/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_LOGGING_ALWAYS_ON_LOGGER_H_
#define MIR_LOGGING_ALWAYS_ON_LOGGER_H_

#include "mir/logging/logger.h"

namespace mir
{
namespace logging
{

class AlwaysOnLogger final : public Logger
{
public:
    static AlwaysOnLogger& instance()
    {
        static AlwaysOnLogger always_on_logger;

        return always_on_logger;
    }

    void log(Severity severity, const std::string& message, const std::string& component) override;
    void log() {}

/*private:*/
    AlwaysOnLogger() {}
};

}
}

#endif // MIR_LOGGING_ALWAYS_ON_LOGGER_H_
