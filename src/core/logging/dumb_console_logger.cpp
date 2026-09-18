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

#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/event.h>

#include <iostream>
#include <ranges>

namespace ml = mir::logging;

void ml::DumbConsoleLogger::log(Event const& event)
{
    std::ostream& out = event.severity() < Severity::informational ? std::cerr : std::cout;

    format_message(
        out,
        event.severity(),
        event.message(),
        event.tags() |
            std::views::transform([](Tag const& tag) { return tag::name(tag); }) |
            std::views::join_with(':') |
            std::ranges::to<std::string>());
}

mir::logging::DumbConsoleLogger::~DumbConsoleLogger()
{
}
