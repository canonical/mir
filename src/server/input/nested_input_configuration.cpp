/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/input/nested_input_configuration.h"

#include <boost/exception/all.hpp>
#include <stdexcept>

namespace mi = mir::input;

mi::NestedInputConfiguration::NestedInputConfiguration(
    std::shared_ptr<EventFilter> const& event_filter,
    std::shared_ptr<InputRegion> const& input_region,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<InputReport> const& input_report) :
event_filter(event_filter),
input_region(input_region),
cursor_listener(cursor_listener),
input_report(input_report)
{
}

mi::NestedInputConfiguration::~NestedInputConfiguration()
{
}

auto mi::NestedInputConfiguration::the_input_registrar()
->     std::shared_ptr<surfaces::InputRegistrar>
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Nested mode input not implemented"));
}

auto mi::NestedInputConfiguration::the_input_targeter()
-> std::shared_ptr<shell::InputTargeter>
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Nested mode input not implemented"));
}

auto mi::NestedInputConfiguration::the_input_manager()
-> std::shared_ptr<InputManager>
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Nested mode input not implemented"));
}

void mi::NestedInputConfiguration::set_input_targets(std::shared_ptr<InputTargets> const& /*targets*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Nested mode input not implemented"));
}
