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

#include <cstring>

// Only MirPointerEvent and MirTouchEvent are typedefed in the public API
typedef struct MirMotionEvent MirMotionEvent;

struct MirEvent
{
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

    MirInputDeviceStateEvent* to_input_device_state();
    MirInputDeviceStateEvent const* to_input_device_state() const;

    MirEvent* clone() const;

    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);

protected:
    MirEvent() = default;
    explicit MirEvent(MirEventType type);
    MirEvent(MirEvent const& event) = default;
    MirEvent& operator=(MirEvent const& event) = default;

    MirEventType type_;
};

namespace mir
{
namespace event
{
template<typename Data>
char const* consume(char const* pos, Data& data)
{
    std::memcpy(&data, pos, sizeof data);
    return pos + sizeof data;
}

template<typename T>
void assert_type_is_trivially_copyable()
{
#if __GNUC__ >= 5
    static_assert(std::is_trivially_copyable<T>::value, "");
#else
    static_assert(__has_trivial_copy(T), "");
#endif
}

// T needs to be trivially copyable
// vivid wont allow a std::is_trivially_copyable
template<typename T>
mir::EventUPtr deserialize_from(std::string const& bytes)
{
    assert_type_is_trivially_copyable<T>();

    T* t = new T;
    memcpy(t, bytes.data(), bytes.size());

    return mir::EventUPtr(t, [](MirEvent* e) { delete e; });
}

template<typename T>
std::string serialize_from(MirEvent const* ev)
{
    assert_type_is_trivially_copyable<T>();

    std::string encoded_bytes;
    encoded_bytes.append(reinterpret_cast<char const*>(ev), sizeof(T));

    return encoded_bytes;
}

template<typename T>
MirEvent* deep_copy(MirEvent const* ev)
{
    assert_type_is_trivially_copyable<T>();

    T* t = new T;
    memcpy(t, ev, sizeof(T));

    return t;
}
}
}

#endif /* MIR_COMMON_EVENT_H_ */
