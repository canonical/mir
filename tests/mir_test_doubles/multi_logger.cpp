/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/test/doubles/multi_logger.h"

#include <boost/throw_exception.hpp>

#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;
namespace mtd = mir::test::doubles;

void mtd::MultiLogger::add(std::shared_ptr<mir::logging::Logger> logger)
{
    loggers.push_back(std::move(logger));
}

void mtd::MultiLogger::log(mir::logging::Severity severity,
                          const std::string& message,
                          const std::string& component)
{
    for (auto it = loggers.begin(); it != loggers.end();)
    {
        try
        {
            (*it)->log(severity, message, component);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            it = loggers.erase(it);
            continue;
        }
        it++;
    }
}
