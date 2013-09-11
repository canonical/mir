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
#include "mir/input/nested_input_relay.h"

#include <InputDispatcher.h>

namespace mi = mir::input;

mi::NestedInputConfiguration::NestedInputConfiguration(
    std::shared_ptr<NestedInputRelay> const& input_relay,
    std::shared_ptr<EventFilter> const& event_filter,
    std::shared_ptr<InputRegion> const& input_region,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<InputReport> const& input_report) :
    android::DispatcherInputConfiguration(event_filter, input_region, cursor_listener, input_report),
    input_relay(input_relay)
{
}

mi::NestedInputConfiguration::~NestedInputConfiguration()
{
}

droidinput::sp<droidinput::InputDispatcherInterface> mi::NestedInputConfiguration::the_dispatcher()
{
    return dispatcher(
        [this]()
        {
            auto const result = new droidinput::InputDispatcher(the_dispatcher_policy(), input_report);
            input_relay->set_dispatcher(result);

            return result;
        });
}
