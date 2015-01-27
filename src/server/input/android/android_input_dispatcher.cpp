/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER 

#include "android_input_dispatcher.h"
#include "android_input_constants.h"
#include "android_input_thread.h"

#include <InputListener.h> // NotifyArgs
#include <InputDispatcher.h>

#include <boost/exception/all.hpp>
#include <stdexcept>

namespace mia = mir::input::android;
namespace droidinput = ::android;

mia::AndroidInputDispatcher::AndroidInputDispatcher(
    std::shared_ptr<droidinput::InputDispatcherInterface> const& dispatcher,
    std::shared_ptr<mia::InputThread> const& thread)
    : dispatcher(dispatcher), dispatcher_thread(thread)
{
}

mia::AndroidInputDispatcher::~AndroidInputDispatcher()
{
    // It is safe to call stop(), even if we haven't been started at all,
    // or we have been previously started and stopped manually.
    stop();
}

void mia::AndroidInputDispatcher::configuration_changed(nsecs_t when)
{
    droidinput::NotifyConfigurationChangedArgs args(when);
    dispatcher->notifyConfigurationChanged(&args);
}

void mia::AndroidInputDispatcher::device_reset(int32_t device_id, nsecs_t when)
{
    droidinput::NotifyDeviceResetArgs args(when, device_id);
    dispatcher->notifyDeviceReset(&args);
}

void mia::AndroidInputDispatcher::dispatch(MirEvent const& event)
{
    static auto const policy_flags = 0;

    switch (event.type)
    {
    case mir_event_type_key:
    {
        droidinput::NotifyKeyArgs const notify_key_args(
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
        std::vector<droidinput::PointerProperties> pointer_properties(event.motion.pointer_count);
        std::vector<droidinput::PointerCoords> pointer_coords(event.motion.pointer_count);

        for(auto i = 0U; i != event.motion.pointer_count; ++i)
        {
            pointer_properties[i].id = event.motion.pointer_coordinates[i].id;
            pointer_properties[i].toolType = event.motion.pointer_coordinates[i].tool_type;

            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_X, event.motion.pointer_coordinates[i].x);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_Y, event.motion.pointer_coordinates[i].y);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, event.motion.pointer_coordinates[i].pressure);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_SIZE, event.motion.pointer_coordinates[i].size);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, event.motion.pointer_coordinates[i].touch_major);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, event.motion.pointer_coordinates[i].touch_minor);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_VSCROLL, event.motion.pointer_coordinates[i].vscroll);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_HSCROLL, event.motion.pointer_coordinates[i].hscroll);
            pointer_coords[i].setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, event.motion.pointer_coordinates[i].orientation);
        }

        droidinput::NotifyMotionArgs const notify_motion_args(
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
}

void mia::AndroidInputDispatcher::stop()
{
    dispatcher_thread->request_stop();
    dispatcher->setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen);
    dispatcher_thread->join();
}

void mia::AndroidInputDispatcher::start()
{
    dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
    dispatcher->setInputFilterEnabled(true);
    dispatcher_thread->start();
}
