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

#include "mir/events/motion_event.h"

MirMotionEvent::MirMotionEvent() :
    MirInputEvent(mir_event_type_motion)
{
}

int32_t MirMotionEvent::device_id() const
{
    return device_id_;
}

void MirMotionEvent::set_device_id(int32_t id)
{
    device_id_ = id;
}

int32_t MirMotionEvent::source_id() const
{
    return source_id_;
}

void MirMotionEvent::set_source_id(int32_t id)
{
    source_id_ = id;
}

MirInputEventModifiers MirMotionEvent::modifiers() const
{
    return modifiers_;
}

void MirMotionEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    modifiers_ = modifiers;
}

MirPointerButtons MirMotionEvent::buttons() const
{
    return buttons_;
}

void MirMotionEvent::set_buttons(MirPointerButtons buttons)
{
    buttons_ = buttons;
}

std::chrono::nanoseconds MirMotionEvent::event_time() const
{
    return event_time_;
}

void MirMotionEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event_time_ = event_time;
}

mir::cookie::Blob MirMotionEvent::cookie() const
{
    return cookie_;
}

void MirMotionEvent::set_cookie(mir::cookie::Blob const& blob)
{
    cookie_ = blob;
}

size_t MirMotionEvent::pointer_count() const
{
    return pointer_count_;
}

void MirMotionEvent::set_pointer_count(size_t count)
{
    pointer_count_ = count;
}

int MirMotionEvent::id(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].id;
}

void MirMotionEvent::set_id(size_t index, int id)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].id = id;
}

float MirMotionEvent::x(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].x;
}

void MirMotionEvent::set_x(size_t index, float x)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].x = x;
}

float MirMotionEvent::y(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].y;
}

void MirMotionEvent::set_y(size_t index, float y)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].y = y;
}

float MirMotionEvent::dx(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].dx;
}

void MirMotionEvent::set_dx(size_t index, float dx)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].dx = dx;
}

float MirMotionEvent::dy(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].dy;
}

void MirMotionEvent::set_dy(size_t index, float dy)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].dy = dy;
}

float MirMotionEvent::touch_major(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].touch_major;
}

void MirMotionEvent::set_touch_major(size_t index, float major)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].touch_major = major;
}

float MirMotionEvent::touch_minor(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].touch_minor;
}

void MirMotionEvent::set_touch_minor(size_t index, float minor)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].touch_minor = minor;
}

float MirMotionEvent::size(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].size;
}

void MirMotionEvent::set_size(size_t index, float size)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].size = size;
}

float MirMotionEvent::pressure(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].pressure;
}

void MirMotionEvent::set_pressure(size_t index, float pressure)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].pressure = pressure;
}

float MirMotionEvent::orientation(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].orientation;
}

void MirMotionEvent::set_orientation(size_t index, float orientation)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].orientation = orientation;
}

float MirMotionEvent::vscroll(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].vscroll;
}

void MirMotionEvent::set_vscroll(size_t index, float vscroll)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].vscroll = vscroll;
}

float MirMotionEvent::hscroll(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].hscroll;
}

void MirMotionEvent::set_hscroll(size_t index, float hscroll)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].hscroll = hscroll;
}

MirTouchTooltype MirMotionEvent::tool_type(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].tool_type;
}

void MirMotionEvent::set_tool_type(size_t index, MirTouchTooltype tool_type)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].tool_type = tool_type;
}

int MirMotionEvent::action(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index].action;
}

void MirMotionEvent::set_action(size_t index, int action)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    pointer_coordinates_[index].action = action;
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
