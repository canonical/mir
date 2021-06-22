/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <boost/throw_exception.hpp>
#include "mir/events/touch_event.h"

#include <stdexcept>

MirTouchEvent::MirTouchEvent() : MirInputEvent(mir_input_event_type_touch)
{
}

MirTouchEvent::MirTouchEvent(MirInputDeviceId id,
                             std::chrono::nanoseconds timestamp,
                             std::vector<uint8_t> const& cookie,
                             MirInputEventModifiers modifiers,
                             std::vector<mir::events::ContactState> const& contacts)
    : MirInputEvent(mir_input_event_type_touch, id, timestamp, modifiers, cookie),
      contacts{contacts}
{
}

auto MirTouchEvent::clone() const -> MirTouchEvent*
{
    return new MirTouchEvent{*this};
}

size_t MirTouchEvent::pointer_count() const
{
    return contacts.size();
}

void MirTouchEvent::set_pointer_count(size_t count)
{
    return contacts.resize(count);
}

void MirTouchEvent::throw_if_out_of_bounds(size_t index) const
{
    if (index > pointer_count())
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));
}

int MirTouchEvent::id(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].touch_id;
}

void MirTouchEvent::set_id(size_t index, int id)
{
    throw_if_out_of_bounds(index);

    contacts[index].touch_id = id;
}

float MirTouchEvent::x(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].x;
}

void MirTouchEvent::set_x(size_t index, float x)
{
    throw_if_out_of_bounds(index);

    contacts[index].x = x;
}

float MirTouchEvent::y(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].y;
}

void MirTouchEvent::set_y(size_t index, float y)
{
    throw_if_out_of_bounds(index);

    contacts[index].y = y;
}

float MirTouchEvent::touch_major(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].touch_major;
}

void MirTouchEvent::set_touch_major(size_t index, float major)
{
    throw_if_out_of_bounds(index);

    contacts[index].touch_major = major;
}

float MirTouchEvent::touch_minor(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].touch_minor;
}

void MirTouchEvent::set_touch_minor(size_t index, float minor)
{
    throw_if_out_of_bounds(index);

    contacts[index].touch_minor = minor;
}

float MirTouchEvent::pressure(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].pressure;
}

void MirTouchEvent::set_pressure(size_t index, float pressure)
{
    throw_if_out_of_bounds(index);

    contacts[index].pressure = pressure;
}

float MirTouchEvent::orientation(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].orientation;
}

void MirTouchEvent::set_orientation(size_t index, float orientation)
{
    throw_if_out_of_bounds(index);

    contacts[index].orientation = orientation;
}

MirTouchTooltype MirTouchEvent::tool_type(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].tooltype;
}

void MirTouchEvent::set_tool_type(size_t index, MirTouchTooltype tool_type)
{
    throw_if_out_of_bounds(index);

    contacts[index].tooltype = tool_type;
}

MirTouchAction MirTouchEvent::action(size_t index) const
{
    throw_if_out_of_bounds(index);

    return contacts[index].action;
}

void MirTouchEvent::set_action(size_t index, MirTouchAction action)
{
    throw_if_out_of_bounds(index);

    contacts[index].action = action;
}
