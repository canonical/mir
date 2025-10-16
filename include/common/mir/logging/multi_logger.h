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

#ifndef MIR_LOGGING_MULTI_LOGGER_H_
#define MIR_LOGGING_MULTI_LOGGER_H_

#include <mir/logging/logger.h>

#include <initializer_list>
#include <string>
#include <vector>

namespace mir
{
namespace logging
{
class MultiLogger : public mir::logging::Logger
{
public:
    MultiLogger(std::initializer_list<std::shared_ptr<Logger>> loggers);

protected:
    void log(mir::logging::Severity severity, std::string const& message, std::string const& component) override;

private:
    std::vector<std::shared_ptr<mir::logging::Logger>> const loggers;
};
}
}

#endif // MIR_LOGGING_MULTI_LOGGER_H_
