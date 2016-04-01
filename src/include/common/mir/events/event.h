/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COMMON_EVENT_H_
#define MIR_COMMON_EVENT_H_

#include "mir_toolkit/event.h"
#include "mir/events/event_builders.h"
#include "mir_event.capnp.h"

#include <capnp/message.h>

#include <cstring>

// Only MirPointerEvent and MirTouchEvent are typedefed in the public API
typedef struct MirMotionEvent MirMotionEvent;

struct MirEvent
{
    MirEvent(MirEvent const& event);
    MirEvent& operator=(MirEvent const& event);

    MirEventType type() const;

    MirInputEvent* to_input();
    MirInputEvent const* to_input() const;

    MirInputConfigurationEvent* to_input_configuration();
    MirInputConfigurationEvent const* to_input_configuration() const;

    MirSurfaceEvent* to_surface();
    MirSurfaceEvent const* to_surface() const;

    MirResizeEvent* to_resize();
    MirResizeEvent const* to_resize() const;

    MirPromptSessionEvent* to_prompt_session();
    MirPromptSessionEvent const* to_prompt_session() const;

    MirOrientationEvent* to_orientation();
    MirOrientationEvent const* to_orientation() const;

    MirCloseSurfaceEvent* to_close_surface();
    MirCloseSurfaceEvent const* to_close_surface() const;

    MirKeymapEvent* to_keymap();
    MirKeymapEvent const* to_keymap() const;

    MirSurfaceOutputEvent* to_surface_output();
    MirSurfaceOutputEvent const* to_surface_output() const;

    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event); 

protected:
    MirEvent() = default;

    ::capnp::MallocMessageBuilder message;
    mir::capnp::Event::Builder event{message.initRoot<mir::capnp::Event>()};
};

#endif /* MIR_COMMON_EVENT_H_ */
