/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "input_receiver_report.h"
#include "mir/report/lttng/mir_tracepoint.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "input_receiver_report_tp.h"

void mir::client::lttng::InputReceiverReport::received_event(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return;
    auto iev = mir_event_get_input_event(&event);
    switch (mir_input_event_get_type(iev))
    {
    case mir_input_event_type_key:
        report_key(iev);
        break;
    case mir_input_event_type_touch:
        report_touch(iev);
        break;
    case mir_input_event_type_pointer:
        // TODO: Print pointer events
        break;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Unexpected event type"));
    }
}

void mir::client::lttng::InputReceiverReport::report_key(MirInputEvent const* event) const
{
    auto kev = mir_input_event_get_keyboard_event(event);

    mir_tracepoint(mir_client_input_receiver, key_event, mir_input_event_get_device_id(event),
                   mir_keyboard_event_action(kev),
                   mir_keyboard_event_modifiers(kev),
                   mir_keyboard_event_key_code(kev),
                   mir_keyboard_event_scan_code(kev),
                   mir_input_event_get_event_time(event));
}

void mir::client::lttng::InputReceiverReport::report_touch(MirInputEvent const* event) const
{
    auto tev = mir_input_event_get_touch_event(event);
    
    mir_tracepoint(mir_client_input_receiver, touch_event, mir_input_event_get_device_id(event),  
                   mir_touch_event_modifiers(tev), mir_input_event_get_event_time(event));

    for (unsigned int i = 0; i < mir_touch_event_point_count(tev); i++)
    {
        mir_tracepoint(mir_client_input_receiver, touch_event_coordinate,
                       mir_touch_event_id(tev, i),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_x),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_y),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_major),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_minor),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_size),
                       mir_touch_event_axis_value(tev, i, mir_touch_axis_pressure));
    }
}

