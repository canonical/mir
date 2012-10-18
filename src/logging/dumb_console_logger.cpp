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

#include "mir/logging/dumb_console_logger.h"

#include <iostream>

namespace ml = mir::logging;

ml::DumbConsoleLogger::DumbConsoleLogger()
{
}

void ml::DumbConsoleLogger::log(ml::Logger::Severity severity,
                                const std::string& message,
                                const std::string& component)
{

    static const char* lut[8] = 
            {
                "EE",
                "AA",
                "CC",
                "EE",
                "WW",
                "NN",
                "II",
                "DD"
            };

    std::ostream& out = std::cout;

    switch(severity)
    {
        case ml::Logger::emergency:            
        case ml::Logger::alert:
        case ml::Logger::critical:
        case ml::Logger::error:
            //out = std::cerr;
            break;
        case ml::Logger::warning:
        case ml::Logger::notice:
        case ml::Logger::informational:
            //out = std::cout;
            break;
        case ml::Logger::debug:
            //out = std::clog;
            break;
        default:
            return;
    }

    out << "[" << lut[severity] << ", " << component << "] " 
        << message << "\n"; 
}
