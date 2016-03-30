/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

// ==================================
// 
// Direct access to MirEvent deprecated. Prefer mir_event_* family of functions.
//
// ==================================

#ifndef MIR_COMMON_EVENT_PRIVATE_H_
#define MIR_COMMON_EVENT_PRIVATE_H_

#include <stddef.h>
#include <stdint.h>
#include "mir_toolkit/event.h"
#include "mir_toolkit/common.h"
#include "mir/cookie/blob.h"

#include "mir/events/event_builders.h"

#include <xkbcommon/xkbcommon.h>
#include <array>
#include <chrono>

/* TODO: To the moon. */
#define MIR_INPUT_EVENT_MAX_POINTER_COUNT 16

typedef struct MirMotionEvent MirMotionEvent;

struct MirEvent
{
    MirEventType type;

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

    MirEvent* clone() const;

    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);

protected:
    MirEvent() = default;
    MirEvent(MirEvent const& event) = default;
    MirEvent& operator=(MirEvent const& event) = default;
};

struct MirInputEvent : MirEvent
{
    MirKeyboardEvent* to_keyboard();
    MirKeyboardEvent const* to_keyboard() const;

    MirMotionEvent* to_motion();
    MirMotionEvent const* to_motion() const;

protected:
    MirInputEvent() = default;
    MirInputEvent(MirInputEvent const& event) = default;
    MirInputEvent& operator=(MirInputEvent const& event) = default;
};

// PRIVATE
// Direct access to MirKeyboardEvent is deprecated. Please use mir_event_get_input_event
// and the mir_input_event* family of functions.
struct MirKeyboardEvent : MirInputEvent
{
    int32_t device_id;
    int32_t source_id;
    MirKeyboardAction action;
    MirInputEventModifiers modifiers;

    int32_t key_code;
    int32_t scan_code;

    std::chrono::nanoseconds event_time;
    mir::cookie::Blob cookie;
};

struct MirMotionPointer
{
    int id;
    float x;
    float y;
    float dx;
    float dy;
    float touch_major;
    float touch_minor;
    float size;
    float pressure;
    float orientation;
    float vscroll;
    float hscroll;
    MirTouchTooltype tool_type;

    // TODO: We would like to store this as a MirTouchAction but we still encode pointer actions
    // here as well.
    int action;
};

// PRIVATE
// Direct access to MirMotionEvent is deprecated. Please use mir_event_get_input_event
// and the mir_input_event* family of functions.
struct MirMotionEvent : MirInputEvent
{
    int32_t device_id;
    int32_t source_id;

    MirInputEventModifiers modifiers;

    MirPointerButtons buttons;
    std::chrono::nanoseconds event_time;
    mir::cookie::Blob cookie;

    size_t pointer_count;
    MirMotionPointer pointer_coordinates[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    /* "_coordinates" is a misnomer here because there's plenty more info than
       just coordinates, but renaming it accurately would be an API break */

    MirTouchEvent* to_touch();
    MirTouchEvent const* to_touch() const;

    MirPointerEvent* to_pointer();
    MirPointerEvent const* to_pointer() const;
};

// These are left empty as they are just aliases for a MirMotionEvent,
// but nice to be implicit cast to the base class
struct MirTouchEvent : MirMotionEvent
{
};

struct MirPointerEvent : MirMotionEvent
{
};
 
struct MirInputConfigurationEvent : MirEvent
{
    MirInputConfigurationAction action;
    std::chrono::nanoseconds when;
    MirInputDeviceId id;
};

struct MirSurfaceEvent : MirEvent
{
    int id;
    MirSurfaceAttrib attrib;
    int value;
};

struct MirResizeEvent : MirEvent
{
    int surface_id;
    int width;
    int height;
};

struct MirPromptSessionEvent : MirEvent
{
    MirPromptSessionState new_state;
};

struct MirOrientationEvent : MirEvent
{
    int surface_id;
    MirOrientation direction;
};

struct MirCloseSurfaceEvent : MirEvent
{
    int surface_id;
};

struct MirKeymapEvent : MirEvent
{
    int surface_id;
    MirInputDeviceId device_id;
    char const* buffer;
    size_t size;

    /* FIXME Need to handle the special case of Keymap events due to char const* buffer */
    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);
    MirEvent* clone() const;
};

struct MirSurfaceOutputEvent : MirEvent
{
    int surface_id;
    int dpi;
    float scale;
    MirFormFactor form_factor;
    uint32_t output_id;
};

#endif /* MIR_COMMON_EVENT_PRIVATE_H_ */
