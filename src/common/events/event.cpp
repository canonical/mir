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

#include <capnp/serialize.h>

MirEvent::MirEvent(MirEvent const& e)
{
    auto reader = e.event.asReader();
    message.setRoot(reader);
    event = message.getRoot<mir::capnp::Event>();
}

MirEvent& MirEvent::operator=(MirEvent const& e)
{
    auto reader = e.event.asReader();
    message.setRoot(reader);
    event = message.getRoot<mir::capnp::Event>();
    return *this;
}

mir::EventUPtr MirEvent::deserialize(std::string const& bytes)
{
    auto e = mir::EventUPtr(new MirEvent, [](MirEvent* ev) { delete ev; });
    kj::ArrayPtr<::capnp::word const> words(reinterpret_cast<::capnp::word const*>(
        bytes.data()), bytes.size() / sizeof(::capnp::word));

    initMessageBuilderFromFlatArrayCopy(words, e->message);
    e->event = e->message.getRoot<mir::capnp::Event>();

    return e;
}

std::string MirEvent::serialize(MirEvent const* event)
{
	std::string output;
	auto flat_event =::capnp::messageToFlatArray(const_cast<MirEvent*>(event)->message);

	for (auto const& c : flat_event.asBytes())
	{
		output += c;
	}

	return output;
}

MirEventType MirEvent::type() const
{
    switch (event.asReader().which())
    {
    case mir::capnp::Event::Which::KEY:
        return mir_event_type_key;
    case mir::capnp::Event::Which::MOTION_SET:
        return mir_event_type_motion;
    case mir::capnp::Event::Which::SURFACE:
        return mir_event_type_surface;
    case mir::capnp::Event::Which::RESIZE:
        return mir_event_type_resize;
    case mir::capnp::Event::Which::PROMPT_SESSION:
        return mir_event_type_prompt_session_state_change;
    case mir::capnp::Event::Which::ORIENTATION:
        return mir_event_type_orientation;
    case mir::capnp::Event::Which::CLOSE_SURFACE:
        return mir_event_type_close_surface;
    case mir::capnp::Event::Which::KEYMAP:
        return mir_event_type_keymap;
    case mir::capnp::Event::Which::INPUT_CONFIGURATION:
        return mir_event_type_input_configuration;
    case mir::capnp::Event::Which::SURFACE_OUTPUT:
        return mir_event_type_surface_output;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Unknown type, major error"));
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
