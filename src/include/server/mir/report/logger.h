/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_REPORT_LOGGER_H_
#define MIR_REPORT_LOGGER_H_

#include "mir/logging/logger.h"
#include <memory>
#include <string>

namespace mir
{
namespace logging
{

void log(Logger::Severity severity, const std::string& message);
void set_logger(std::shared_ptr<Logger> const& new_logger);

}
}

#endif // MIR_REPORT_LOGGER_H_
