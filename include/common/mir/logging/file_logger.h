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

#ifndef MIR_LOGGING_FILE_LOGGER_H_
#define MIR_LOGGING_FILE_LOGGER_H_

#include <mir/logging/logger.h>

#include <atomic>
#include <fstream>
#include <string>

namespace mir
{
namespace logging
{
class FileLogger : public mir::logging::Logger
{
public:
    FileLogger(std::ofstream stream);

protected:
    void log(mir::logging::Severity severity, std::string const& message, std::string const& component) override;

private:
    std::ofstream file;
    std::atomic_bool file_bad{false};
};
}
}

#endif // MIR_LOGGING_FILE_LOGGER_H_
