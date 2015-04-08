/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "default_input_dispatcher.h"

#include <boost/throw_exception.hpp>
#include <stdexcept.h>

namespace mi = mir::input;

mi::DefaultInputDispatcher::DefaultInputDispatcher(std::shared_ptr<mi::Scene> const& scene)
    : scene(scene)
{
    (void) scene;
}

void mi::DefaultInputDispatcher::configuration_changed(std::chrono::nanoseconds when)
{
    (void) when;
}

void mi::DefaultInputDispatcher::device_reset(int32_t device_id, std::chrono::nanoseconds when)
{
    (void) device_id;
    (void) when;
}

bool mi::DefaultInputDispatcher::dispatch_key(MirKeyEvent *kev)
{
    std::lock_guard<std::mutex> lg(focus_guard);
    auto strong_focus = focus_surface.lock();
    if (!strong_focus)
        return false;

    // TODO: Impl state tracking

    strong_focus->consume(*(reinterpret_cast<MirEvent*>(kev)));
}

bool mi::DefaultInputDispatcher::dispatch_pointer(MirPointerEvent *pev)
{
    (void) pev;
    return false;
}

bool mi::DefaultInputDispatcher::dispatch_touch(MirTouchEvent *tev)
{
    (void) tev;
    return false;
}

bool mi::DefaultInputDispatcher::dispatch(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        BOOST_THROW_EXCEPTION(std::logic_error("InputDispatcher got a non-input event"));
    auto iev = mir_input_event_get_type(&event);
    switch (mir_input_event_get_type(iev))
    {
    case mir_input_event_type_key:
        return dispatch_key(mir_input_event_get_keyboard_event(iev));
    case mir_input_event_type_touch:
        return dispatch_touch(mir_input_event_get_touch_event(iev));
    case mir_input_event_type_pointer:
        return dispatch_pointer(mir_input_event_get_pointer_event(iev));
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("InputDispatcher got an input event of unknown type"));
    }
    
    return true;
}

void mi::DefaultInputDispatcher::start()
{
    // TODO: Trigger hover here?
}

void mi::DefaultInputDispatcher::stop()
{
}

void mi::DefaultInputDispatcher::set_focus(std::shared_ptr<input::Surface> const& target)
{
    std::lock_guard<std::mutex> lg(focus_mutex);

    focus_surface = target;
}

