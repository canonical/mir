/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_EVENT_H_
#define MIR_COMMON_EVENT_H_

#include <mir_toolkit/event.h>
#include <mir/events/event_builders.h>

#include <cstring>

struct MirEvent
{
    virtual auto clone() const -> MirEvent* = 0;
    virtual ~MirEvent() = default;

    MirEventType type() const;

    MirInputEvent* to_input();
    MirInputEvent const* to_input() const;

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

    MirWindowOutputEvent* to_window_output();
    MirWindowOutputEvent const* to_window_output() const;

    MirInputDeviceStateEvent* to_input_device_state();
    MirInputDeviceStateEvent const* to_input_device_state() const;

    MirWindowPlacementEvent* to_window_placement();
    MirWindowPlacementEvent const* to_window_placement() const;

protected:
    MirEvent(MirEventType type);
    MirEvent(MirEvent const& event);

private:
    MirEvent& operator=(MirEvent const& event) = delete;

    MirEventType const type_;
};

#endif /* MIR_COMMON_EVENT_H_ */
