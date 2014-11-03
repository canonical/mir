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

#include "mir/logging/always_on_logger.h"

#include <syslog.h>

namespace ml = mir::logging;

ml::AlwaysOnLogger& ml::AlwaysOnLogger::instance()
{
    static ml::AlwaysOnLogger always_on_logger;

    return always_on_logger;
}

void ml::AlwaysOnLogger::log(ml::Logger::Severity /*severity*/,
                             const std::string& message,
                             const std::string& component)
{
    syslog(LOG_INFO, "%s: %s", component.c_str(), message.c_str());
}
