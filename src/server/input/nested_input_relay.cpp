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

#include "nested_input_relay.h"

#include <InputDispatcher.h>

#include <boost/exception/all.hpp>
#include <stdexcept>

namespace mi = mir::input;

mi::NestedInputRelay::NestedInputRelay()
{
}

mi::NestedInputRelay::~NestedInputRelay() noexcept
{
}

void mi::NestedInputRelay::set_dispatcher(::android::sp<::android::InputDispatcher> const& dispatcher)
{
    this->dispatcher = dispatcher;
}

bool mi::NestedInputRelay::handle(MirEvent const& event)
{
    if (dispatcher == nullptr)
    {
        return false;
    }

    static auto const policy_flags = 0;

    switch (event.type)
    {
    case mir_event_type_key:
    {
        ::android::NotifyKeyArgs const notify_key_args(
            event.key.event_time,
            event.key.device_id,
            event.key.source_id,
            policy_flags,
            event.key.action,
            event.key.flags,
            event.key.key_code,
            event.key.scan_code,
            event.key.modifiers,
            event.key.down_time);

        dispatcher->notifyKey(&notify_key_args);

        break;
    }

    case mir_event_type_motion:
    {
        std::vector<::android::PointerProperties> pointer_properties(event.motion.pointer_count);
        std::vector<::android::PointerCoords> pointer_coords(event.motion.pointer_count);

        for(auto i = 0U; i != event.motion.pointer_count; ++i)
        {
            pointer_properties[i].id = event.motion.pointer_coordinates[i].id;
            pointer_properties[i].toolType = 0;

            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_X, event.motion.pointer_coordinates[i].x);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_Y, event.motion.pointer_coordinates[i].y);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, event.motion.pointer_coordinates[i].pressure);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_SIZE, event.motion.pointer_coordinates[i].size);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, event.motion.pointer_coordinates[i].touch_major);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, event.motion.pointer_coordinates[i].touch_minor);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, event.motion.pointer_coordinates[i].orientation);
        }

        ::android::NotifyMotionArgs const notify_motion_args(
            event.motion.event_time,
            event.motion.device_id,
            event.motion.source_id,
            policy_flags,
            event.motion.action,
            event.motion.flags,
            event.motion.modifiers,
            event.motion.button_state,
            event.motion.edge_flags,
            event.motion.pointer_count,
            pointer_properties.data(),
            pointer_coords.data(),
            event.motion.x_precision,
            event.motion.y_precision,
            event.motion.down_time);

        dispatcher->notifyMotion(&notify_motion_args);

        break;
    }

    case mir_event_type_surface:
        // Just ignore these events: it doesn't make sense to pass them on.
        break;

    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Unhandled event type"));
    }

    return true;
}
