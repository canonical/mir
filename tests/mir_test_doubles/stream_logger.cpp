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

#include "mir/test/doubles/stream_logger.h"

#include <boost/throw_exception.hpp>

#include <iostream>
#include <ctime>
#include <cstdio>

namespace ml = mir::logging;
namespace mtd = mir::test::doubles;

mtd::StreamLogger::StreamLogger(std::unique_ptr<std::ostream> out)
: out(std::move(out))
{
    if (!out || !out->good())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Received invalid stream for logging"});
    }
}

void mtd::StreamLogger::log(mir::logging::Severity severity,
                          const std::string& message,
                          const std::string& component)
{
    if (!out->good())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to write to log stream"});
    }

    ml::format_message(*out, severity, message, component);
}
