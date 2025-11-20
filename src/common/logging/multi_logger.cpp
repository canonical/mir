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

#include <mir/logging/multi_logger.h>

namespace ml = mir::logging;

ml::MultiLogger::MultiLogger(std::initializer_list<std::shared_ptr<Logger>> loggers)
: loggers{loggers}
{
}

void ml::MultiLogger::log(Severity severity,
                          std::string const& message,
                          std::string const& component)
{
    for (auto const& logger : loggers)
    {
        logger->log(severity, message, component);
    }
}
