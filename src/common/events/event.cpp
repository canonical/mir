/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/events/event.h"

#include "mir/events/close_surface_event.h"
#include "mir/events/input_configuration_event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/keymap_event.h"
#include "mir/events/motion_event.h"
#include "mir/events/orientation_event.h"
#include "mir/events/prompt_session_event.h"
#include "mir/events/resize_event.h"
#include "mir/events/surface_event.h"
#include "mir/events/surface_output_event.h"
#include "mir/events/input_device_state_event.h"

MirEvent::MirEvent(MirEventType type) :
    type_(type)
{
}

MirEvent* MirEvent::clone() const
{
    switch (type_)
    {
    case mir_event_type_key:
        return mir::event::deep_copy<MirKeyboardEvent>(this);
    case mir_event_type_motion:
        return mir::event::deep_copy<MirMotionEvent>(this);
    case mir_event_type_surface:
        return mir::event::deep_copy<MirSurfaceEvent>(this);
    case mir_event_type_resize:
        return mir::event::deep_copy<MirResizeEvent>(this);
    case mir_event_type_prompt_session_state_change:
        return mir::event::deep_copy<MirPromptSessionEvent>(this);
    case mir_event_type_orientation:
        return mir::event::deep_copy<MirOrientationEvent>(this);
    case mir_event_type_close_surface:
        return mir::event::deep_copy<MirCloseSurfaceEvent>(this);
    case mir_event_type_input_configuration:
        return mir::event::deep_copy<MirInputConfigurationEvent>(this);
    case mir_event_type_surface_output:
        return mir::event::deep_copy<MirSurfaceOutputEvent>(this);
    case mir_event_type_keymap:
        return to_keymap()->clone();
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to clone event"));
}

mir::EventUPtr MirEvent::deserialize(std::string const& bytes)
{
    auto minimal_event_size = sizeof(MirEventType);
    auto const stream_size = bytes.size();
    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    MirEventType type;
    mir::event::consume(bytes.data(), type);

    switch (type)
    {
    case mir_event_type_key:
        return mir::event::deserialize_from<MirKeyboardEvent>(bytes);
    case mir_event_type_motion:
        return mir::event::deserialize_from<MirMotionEvent>(bytes);
    case mir_event_type_surface:
        return mir::event::deserialize_from<MirSurfaceEvent>(bytes);
    case mir_event_type_resize:
        return mir::event::deserialize_from<MirResizeEvent>(bytes);
    case mir_event_type_prompt_session_state_change:
        return mir::event::deserialize_from<MirPromptSessionEvent>(bytes);
    case mir_event_type_orientation:
        return mir::event::deserialize_from<MirOrientationEvent>(bytes);
    case mir_event_type_close_surface:
        return mir::event::deserialize_from<MirCloseSurfaceEvent>(bytes);
    case mir_event_type_input_configuration:
        return mir::event::deserialize_from<MirInputConfigurationEvent>(bytes);
    case mir_event_type_surface_output:
        return mir::event::deserialize_from<MirSurfaceOutputEvent>(bytes);
    case mir_event_type_keymap:
        return MirKeymapEvent::deserialize(bytes);
    case mir_event_type_input_device_state:
        return MirInputDeviceStateEvent::deserialize(bytes);
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));
}

std::string MirEvent::serialize(MirEvent const* event)
{
    switch (event->type())
    {
    case mir_event_type_key:
        return mir::event::serialize_from<MirKeyboardEvent>(event);
    case mir_event_type_motion:
        return mir::event::serialize_from<MirMotionEvent>(event);
    case mir_event_type_surface:
        return mir::event::serialize_from<MirSurfaceEvent>(event);
    case mir_event_type_resize:
        return mir::event::serialize_from<MirResizeEvent>(event);
    case mir_event_type_prompt_session_state_change:
        return mir::event::serialize_from<MirPromptSessionEvent>(event);
    case mir_event_type_orientation:
        return mir::event::serialize_from<MirOrientationEvent>(event);
    case mir_event_type_close_surface:
        return mir::event::serialize_from<MirCloseSurfaceEvent>(event);
    case mir_event_type_input_configuration:
        return mir::event::serialize_from<MirInputConfigurationEvent>(event);
    case mir_event_type_surface_output:
        return mir::event::serialize_from<MirSurfaceOutputEvent>(event);
    case mir_event_type_keymap:
        return MirKeymapEvent::serialize(event);
    case mir_event_type_input_device_state:
        return MirInputDeviceStateEvent::serialize(event);
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to serialize event"));
}

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

MirInputConfigurationEvent* MirEvent::to_input_configuration()
{
    return static_cast<MirInputConfigurationEvent*>(this);
}

MirInputConfigurationEvent const* MirEvent::to_input_configuration() const
{
    return static_cast<MirInputConfigurationEvent const*>(this);
}

MirSurfaceEvent* MirEvent::to_surface()
{
    return static_cast<MirSurfaceEvent*>(this);
}

MirSurfaceEvent const* MirEvent::to_surface() const
{
    return static_cast<MirSurfaceEvent const*>(this);
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

MirCloseSurfaceEvent* MirEvent::to_close_surface()
{
    return static_cast<MirCloseSurfaceEvent*>(this);
}

MirCloseSurfaceEvent const* MirEvent::to_close_surface() const
{
    return static_cast<MirCloseSurfaceEvent const*>(this);
}

MirKeymapEvent* MirEvent::to_keymap()
{
    return static_cast<MirKeymapEvent*>(this);
}

MirKeymapEvent const* MirEvent::to_keymap() const
{
    return static_cast<MirKeymapEvent const*>(this);
}

MirSurfaceOutputEvent* MirEvent::to_surface_output()
{
    return static_cast<MirSurfaceOutputEvent*>(this);
}

MirSurfaceOutputEvent const* MirEvent::to_surface_output() const
{
    return static_cast<MirSurfaceOutputEvent const*>(this);
}

MirInputDeviceStateEvent* MirEvent::to_input_device_state()
{
    return static_cast<MirInputDeviceStateEvent*>(this);
}

MirInputDeviceStateEvent const* MirEvent::to_input_device_state() const
{
    return static_cast<MirInputDeviceStateEvent const*>(this);
}
