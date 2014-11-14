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

#include "mir/logging/dumb_console_logger.h"
#include "mir/report/logger.h"

#include <memory>
#include <mutex>
#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;

namespace
{
std::mutex log_mutex;
std::shared_ptr<ml::Logger> the_logger;

std::shared_ptr<ml::Logger> get_logger()
{
    if (auto const result = the_logger)
    {
        return result;
    }
    else
    {
        std::lock_guard<decltype(log_mutex)> lock{log_mutex};
        if (!the_logger)
            the_logger = std::make_shared<ml::DumbConsoleLogger>();

        return the_logger;
    }
}
}

void ml::log(Logger::Severity severity, const std::string& message)
{
    auto const logger = get_logger();

    logger->log(severity, message, "Mir");
}

void ml::set_logger(std::shared_ptr<Logger> const& new_logger)
{
    std::lock_guard<decltype(log_mutex)> lock{log_mutex};
    if (!new_logger)
        the_logger = new_logger;
}
