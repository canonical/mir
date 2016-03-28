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

#include "mir/events/event_private.h"

#include <cstring>

#include <boost/throw_exception.hpp>

//TODO Temp functions until we move to capnproto which will do this for us!
namespace
{
void delete_keymap_event(MirEvent* e)
{
    if (e && e->to_keymap()->buffer)
    {
        std::free(const_cast<char*>(e->to_keymap()->buffer));
    }

    delete e;
}

// consume/encode needed for keymap!
template<typename Data>
char const* consume(char const* pos, Data& data)
{
    std::memcpy(&data, pos, sizeof data);
    return pos + sizeof data;
}

template<typename Data>
void encode(std::string& encoded, Data const& data)
{
    encoded.append(reinterpret_cast<char const*>(&data), sizeof data);
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

MirEvent* MirEvent::clone() const
{
    switch (type)
    {
    case mir_event_type_key:
        return deep_copy<MirKeyboardEvent>(this);
    case mir_event_type_motion:
        return deep_copy<MirMotionEvent>(this);
    case mir_event_type_surface:
        return deep_copy<MirSurfaceEvent>(this);
    case mir_event_type_resize:
        return deep_copy<MirResizeEvent>(this);
    case mir_event_type_prompt_session_state_change:
        return deep_copy<MirPromptSessionEvent>(this);
    case mir_event_type_orientation:
        return deep_copy<MirOrientationEvent>(this);
    case mir_event_type_close_surface:
        return deep_copy<MirCloseSurfaceEvent>(this);
    case mir_event_type_input_configuration:
        return deep_copy<MirInputConfigurationEvent>(this);
    case mir_event_type_surface_output:
        return deep_copy<MirSurfaceOutputEvent>(this);
    case mir_event_type_keymap:
        return to_keymap()->clone();
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to clone event"));
}

MirEvent* MirKeymapEvent::clone() const
{
    auto new_ev = deep_copy<MirKeymapEvent>(this);
    auto keymap = new_ev->to_keymap();

    // malloc to match xkbcommons allocation behavior
    auto buffer = static_cast<char*>(malloc(keymap->size));
    std::memcpy(buffer, keymap->buffer, keymap->size);
    keymap->buffer = buffer;

    return new_ev;
}

mir::EventUPtr MirEvent::deserialize(std::string const& bytes)
{
    // Just getting the type from the raw bytes, discard after
    MirEvent event;
    auto minimal_event_size = sizeof event.type;
    auto const stream_size = bytes.size();
    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    consume(bytes.data(), event.type);

    switch (event.type)
    {
    case mir_event_type_key:
        return deserialize_from<MirKeyboardEvent>(bytes);
    case mir_event_type_motion:
        return deserialize_from<MirMotionEvent>(bytes);
    case mir_event_type_surface:
        return deserialize_from<MirSurfaceEvent>(bytes);
    case mir_event_type_resize:
        return deserialize_from<MirResizeEvent>(bytes);
    case mir_event_type_prompt_session_state_change:
        return deserialize_from<MirPromptSessionEvent>(bytes);
    case mir_event_type_orientation:
        return deserialize_from<MirOrientationEvent>(bytes);
    case mir_event_type_close_surface:
        return deserialize_from<MirCloseSurfaceEvent>(bytes);
    case mir_event_type_input_configuration:
        return deserialize_from<MirInputConfigurationEvent>(bytes);
    case mir_event_type_surface_output:
        return deserialize_from<MirSurfaceOutputEvent>(bytes);
    case mir_event_type_keymap:
        return MirKeymapEvent::deserialize(bytes);
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));
}

// Edge case where we have handle the char* buffer manually!
mir::EventUPtr MirKeymapEvent::deserialize(std::string const& bytes)
{
    auto event = mir::EventUPtr(new MirKeymapEvent, delete_keymap_event);
    auto& keymap = *event->to_keymap();
    auto minimal_event_size = sizeof event->type;
    auto const stream_size = bytes.size();
    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    char const* pos = consume(bytes.data(), event->type);

    minimal_event_size += sizeof keymap.surface_id + sizeof keymap.device_id + sizeof keymap.size;

    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));
    pos = consume(pos, keymap.surface_id);
    pos = consume(pos, keymap.device_id);
    pos = consume(pos, keymap.size);

    minimal_event_size += keymap.size;

    if (stream_size < minimal_event_size)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to deserialize event"));

    auto buffer = static_cast<char*>(std::malloc(keymap.size));
    std::memcpy(buffer, pos, keymap.size);
    keymap.buffer = buffer;

    return event;
}

std::string MirEvent::serialize(MirEvent const* event)
{
    switch (event->type)
    {
    case mir_event_type_key:
        return serialize_from<MirKeyboardEvent>(event);
    case mir_event_type_motion:
        return serialize_from<MirMotionEvent>(event);
    case mir_event_type_surface:
        return serialize_from<MirSurfaceEvent>(event);
    case mir_event_type_resize:
        return serialize_from<MirResizeEvent>(event);
    case mir_event_type_prompt_session_state_change:
        return serialize_from<MirPromptSessionEvent>(event);
    case mir_event_type_orientation:
        return serialize_from<MirOrientationEvent>(event);
    case mir_event_type_close_surface:
        return serialize_from<MirCloseSurfaceEvent>(event);
    case mir_event_type_input_configuration:
        return serialize_from<MirInputConfigurationEvent>(event);
    case mir_event_type_surface_output:
        return serialize_from<MirSurfaceOutputEvent>(event);
    case mir_event_type_keymap:
        return MirKeymapEvent::serialize(event);
    case mir_event_type_input:
    default:
        break;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to serialize event"));
}

// Edge case where we have handle the char* buffer manually!
std::string MirKeymapEvent::serialize(MirEvent const* event)
{
    std::string encoded;
    auto keymap = event->to_keymap();
    encoded.reserve(sizeof event->type +
                    sizeof keymap->surface_id +
                    sizeof keymap->device_id +
                    sizeof keymap->size +
                    keymap->size);
    encode(encoded, event->type);
    encode(encoded, keymap->surface_id);
    encode(encoded, keymap->device_id);
    encode(encoded, keymap->size);
    encoded.append(keymap->buffer, keymap->size);

    return encoded;
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

MirKeyboardEvent* MirInputEvent::to_keyboard()
{
    return static_cast<MirKeyboardEvent*>(this);
}

MirKeyboardEvent const* MirInputEvent::to_keyboard() const
{
    return static_cast<MirKeyboardEvent const*>(this);
}

MirMotionEvent* MirInputEvent::to_motion()
{
    return static_cast<MirMotionEvent*>(this);
}

MirMotionEvent const* MirInputEvent::to_motion() const
{
    return static_cast<MirMotionEvent const*>(this);
}

MirTouchEvent* MirMotionEvent::to_touch()
{
    return static_cast<MirTouchEvent*>(this);
}

MirTouchEvent const* MirMotionEvent::to_touch() const
{
    return static_cast<MirTouchEvent const*>(this);
}

MirPointerEvent* MirMotionEvent::to_pointer()
{
    return static_cast<MirPointerEvent*>(this);
}

MirPointerEvent const* MirMotionEvent::to_pointer() const
{
    return static_cast<MirPointerEvent const*>(this);
}
