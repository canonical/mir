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

struct MirEvent
{
    MirEvent(MirEvent const& event);
    MirEvent& operator=(MirEvent const& event);

    MirEventType type() const;

    MirInputEvent* to_input();
    MirInputEvent const* to_input() const;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirInputConfigurationEvent* to_input_configuration();
    MirInputConfigurationEvent const* to_input_configuration() const;
#pragma GCC diagnostic pop

    MirWindowEvent* to_surface();
    MirWindowEvent const* to_surface() const;

    MirResizeEvent* to_resize();
    MirResizeEvent const* to_resize() const;

    MirPromptSessionEvent* to_prompt_session();
    MirPromptSessionEvent const* to_prompt_session() const;

    MirOrientationEvent* to_orientation();
    MirOrientationEvent const* to_orientation() const;

    MirCloseWindowEvent* to_close_window();
    MirCloseWindowEvent const* to_close_window() const;

    MirKeymapEvent* to_keymap();
    MirKeymapEvent const* to_keymap() const;

    MirWindowOutputEvent* to_window_output();
    MirWindowOutputEvent const* to_window_output() const;

    MirInputDeviceStateEvent* to_input_device_state();
    MirInputDeviceStateEvent const* to_input_device_state() const;

    MirWindowPlacementEvent const* to_window_placement() const;

    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);

protected:
    MirEvent() = default;

    ::capnp::MallocMessageBuilder message;
    mir::capnp::Event::Builder event{message.initRoot<mir::capnp::Event>()};
};

#endif /* MIR_COMMON_EVENT_H_ */
