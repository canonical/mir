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

#include "mir/input/nested_input_relay.h"

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

    // TODO it isn't obvious what to pass to injectInputEvent()
    // TODO can reviewers look carefully at these?
    static auto const injector_pid = 0;
    static auto const injector_uid = 0;
    static auto const sync_mode = ::android::INPUT_EVENT_INJECTION_SYNC_NONE;
    static auto const timeout_millis = 0;
    static auto const policy_flags = ::android::POLICY_FLAG_INJECTED | ::android::POLICY_FLAG_TRUSTED;

    switch (event.type)
    {
    case mir_event_type_key:
    {
        ::android::KeyEvent input_event;

        input_event.initialize(
            event.key.device_id,    //int32_t deviceId,
            event.key.source_id,    //int32_t source,
            event.key.action,       //int32_t action,
            event.key.flags,        //int32_t flags,
            event.key.key_code,     //int32_t keyCode,
            event.key.scan_code,    //int32_t scanCode,
            event.key.modifiers,    //int32_t metaState,
            event.key.repeat_count, //int32_t repeatCount,
            event.key.down_time,    //nsecs_t downTime,
            event.key.event_time);  //nsecs_t eventTime

        dispatcher->injectInputEvent(
            &input_event,   //const InputEvent* event,
            injector_pid,   //int32_t injectorPid,
            injector_uid,   //int32_t injectorUid,
            sync_mode,      //int32_t syncMode,
            timeout_millis, //int32_t timeoutMillis,
            policy_flags);  //uint32_t policyFlags
        break;
    }

    case mir_event_type_motion:
    {
        ::android::MotionEvent input_event;

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

        input_event.initialize(
            event.motion.device_id,     //int32_t deviceId,
            event.motion.source_id,     //int32_t source,
            event.motion.action,        //int32_t action,
            event.motion.flags,         //int32_t flags,
            event.motion.edge_flags,    //int32_t edgeFlags,
            event.motion.modifiers,     //int32_t metaState,
            event.motion.button_state,  //int32_t buttonState,
            event.motion.x_offset,      //float xOffset,
            event.motion.y_offset,      //float yOffset,
            event.motion.x_precision,   //float xPrecision,
            event.motion.y_precision,   //float yPrecision,
            event.motion.down_time,     //nsecs_t downTime,
            event.motion.event_time,    //nsecs_t eventTime,
            event.motion.pointer_count, //size_t pointerCount,
            pointer_properties.data(),  //const PointerProperties* pointerProperties,
            pointer_coords.data());     //const PointerCoords* pointerCoords);

        dispatcher->injectInputEvent(
            &input_event,   //const InputEvent* event,
            injector_pid,   //int32_t injectorPid,
            injector_uid,   //int32_t injectorUid,
            sync_mode,      //int32_t syncMode,
            timeout_millis, //int32_t timeoutMillis,
            policy_flags);  //uint32_t policyFlags
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
