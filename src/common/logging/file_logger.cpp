/*
 * Copyright © 2022 Canonical Ltd.
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

#include "mir/logging/file_logger.h"

#include <boost/throw_exception.hpp>

#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;

ml::FileLogger::FileLogger(std::ofstream stream)
: file{std::move(stream)}
{
    if (!file.good())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Received invalid file stream for logging"});
    }
}

void ml::FileLogger::log(Severity severity,
                         const std::string& message,
                         const std::string& component)
{
    if (!file.good())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to write to log file"});
    }

    ml::format_message(file, severity, message, component);
}
