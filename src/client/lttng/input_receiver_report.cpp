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
    switch (event.type)
    {
    case mir_event_type_key:
        report(event.key);
        break;
    case mir_event_type_motion:
        report(event.motion);
        break;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Unexpected event type"));
    }
}

void mir::client::lttng::InputReceiverReport::report(MirKeyEvent const& event) const
{
    mir_tracepoint(mir_client_input_receiver, key_event, event.device_id, event.source_id,
                   static_cast<int>(event.action), static_cast<int>(event.flags), event.modifiers, event.key_code,
                   event.scan_code, event.down_time, event.event_time);
}

void mir::client::lttng::InputReceiverReport::report(MirMotionEvent const& event) const
{
    mir_tracepoint(mir_client_input_receiver, motion_event, event.device_id, event.source_id, event.action,
                   static_cast<int>(event.flags), event.modifiers, event.edge_flags,
                   static_cast<int>(event.button_state), event.down_time, event.event_time);
    for (unsigned int i = 0; i < event.pointer_count; i++)
    {
        mir_tracepoint(mir_client_input_receiver, motion_event_coordinate,
                       event.pointer_coordinates[i].id, event.pointer_coordinates[i].x, event.pointer_coordinates[i].y,
                       event.pointer_coordinates[i].touch_major, event.pointer_coordinates[i].touch_minor,
                       event.pointer_coordinates[i].size, event.pointer_coordinates[i].pressure,
                       event.pointer_coordinates[i].orientation);
    }
}

