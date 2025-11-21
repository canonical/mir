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

#include <mir/logging/file_logger.h>

#include <iostream>

namespace ml = mir::logging;

ml::FileLogger::FileLogger(std::ofstream stream)
: file{std::move(stream)}
{
}

void ml::FileLogger::log(Severity severity,
                         std::string const& message,
                         std::string const& component)
{
    if (file.good())
    {
        format_message(file, severity, message, component);
    }
    else if (!file_bad.load())
    {
        std::cerr << "Failed to write to log file" << std::endl;
        file_bad.store(true);
    }
}
