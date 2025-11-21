/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/events/event.h>
#include <mir/events/close_window_event.h>
#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/events/touch_event.h>
#include <mir/events/orientation_event.h>
#include <mir/events/prompt_session_event.h>
#include <mir/events/resize_event.h>
#include <mir/events/window_event.h>
#include <mir/events/window_output_event.h>
#include <mir/events/input_device_state_event.h>
#include <mir/events/window_placement_event.h>


MirEvent::MirEvent(MirEventType type) :
    type_{type}
{
}

MirEvent::MirEvent(MirEvent const& e) = default;

MirEventType MirEvent::type() const
{
    return type_;
}

MirInputEvent* MirEvent::to_input()
{
    return static_cast<MirInputEvent*>(this);
}

MirInputEvent const* MirEvent::to_input() const
{
    return static_cast<MirInputEvent const*>(this);
}

MirWindowEvent* MirEvent::to_surface()
{
    return static_cast<MirWindowEvent*>(this);
}

MirWindowEvent const* MirEvent::to_surface() const
{
    return static_cast<MirWindowEvent const*>(this);
}

MirResizeEvent* MirEvent::to_resize()
{
    return static_cast<MirResizeEvent*>(this);
}

MirResizeEvent const* MirEvent::to_resize() const
{
    return static_cast<MirResizeEvent const*>(this);
}

MirPromptSessionEvent* MirEvent::to_prompt_session()
{
    return static_cast<MirPromptSessionEvent*>(this);
}

MirPromptSessionEvent const* MirEvent::to_prompt_session() const
{
    return static_cast<MirPromptSessionEvent const*>(this);
}

MirOrientationEvent* MirEvent::to_orientation()
{
    return static_cast<MirOrientationEvent*>(this);
}

MirOrientationEvent const* MirEvent::to_orientation() const
{
    return static_cast<MirOrientationEvent const*>(this);
}

MirCloseWindowEvent* MirEvent::to_close_window()
{
    return static_cast<MirCloseWindowEvent*>(this);
}

MirCloseWindowEvent const* MirEvent::to_close_window() const
{
    return static_cast<MirCloseWindowEvent const*>(this);
}

MirWindowOutputEvent* MirEvent::to_window_output()
{
    return static_cast<MirWindowOutputEvent*>(this);
}

MirWindowOutputEvent const* MirEvent::to_window_output() const
{
    return static_cast<MirWindowOutputEvent const*>(this);
}

MirInputDeviceStateEvent* MirEvent::to_input_device_state()
{
    return static_cast<MirInputDeviceStateEvent*>(this);
}

MirInputDeviceStateEvent const* MirEvent::to_input_device_state() const
{
    return static_cast<MirInputDeviceStateEvent const*>(this);
}
