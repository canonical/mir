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

MirMotionEvent::MirMotionEvent()
{
    event.initMotionSet();
    event.getMotionSet().initMotions(mir::capnp::MotionEventSet::MAX_COUNT);
}

int32_t MirMotionEvent::device_id() const
{
    return event.asReader().getMotionSet().getDeviceId().getId();
}

void MirMotionEvent::set_device_id(int32_t id)
{
    event.getMotionSet().getDeviceId().setId(id);
}

int32_t MirMotionEvent::source_id() const
{
    return event.asReader().getMotionSet().getSourceId();
}

void MirMotionEvent::set_source_id(int32_t id)
{
    event.getMotionSet().setSourceId(id);
}

MirInputEventModifiers MirMotionEvent::modifiers() const
{
    return event.asReader().getMotionSet().getModifiers();
}

void MirMotionEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    event.getMotionSet().setModifiers(modifiers);
}

MirPointerButtons MirMotionEvent::buttons() const
{
    return event.asReader().getMotionSet().getButtons();
}

void MirMotionEvent::set_buttons(MirPointerButtons buttons)
{
    event.getMotionSet().setButtons(buttons);
}

std::chrono::nanoseconds MirMotionEvent::event_time() const
{
    return std::chrono::nanoseconds{event.asReader().getMotionSet().getEventTime().getCount()};
}

void MirMotionEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event.getMotionSet().getEventTime().setCount(event_time.count());
}

std::vector<uint8_t> MirMotionEvent::cookie() const
{
    auto cookie = event.asReader().getMotionSet().getCookie();
    std::vector<uint8_t> vec_cookie(cookie.size());
    std::copy(std::begin(cookie), std::end(cookie), std::begin(vec_cookie));

    return vec_cookie;
}

void MirMotionEvent::set_cookie(std::vector<uint8_t> const& cookie)
{
    ::capnp::Data::Reader cookie_data(cookie.data(), cookie.size());
    event.getMotionSet().setCookie(cookie_data);
}

size_t MirMotionEvent::pointer_count() const
{
    return event.asReader().getMotionSet().getCount();
}

void MirMotionEvent::set_pointer_count(size_t count)
{
    return event.getMotionSet().setCount(count);
}

void MirMotionEvent::throw_if_out_of_bounds(size_t index) const
{
    if (index > event.asReader().getMotionSet().getCount())
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));
}

int MirMotionEvent::id(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getId();
}

void MirMotionEvent::set_id(size_t index, int id)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setId(id);
}

float MirMotionEvent::x(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getX();
}

void MirMotionEvent::set_x(size_t index, float x)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setX(x);
}

float MirMotionEvent::y(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getY();
}

void MirMotionEvent::set_y(size_t index, float y)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setY(y);
}

float MirMotionEvent::dx(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getDx();
}

void MirMotionEvent::set_dx(size_t index, float dx)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setDx(dx);
}

float MirMotionEvent::dy(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getDy();
}

void MirMotionEvent::set_dy(size_t index, float dy)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setDy(dy);
}

float MirMotionEvent::touch_major(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getTouchMajor();
}

void MirMotionEvent::set_touch_major(size_t index, float major)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setTouchMajor(major);
}

float MirMotionEvent::touch_minor(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getTouchMinor();
}

void MirMotionEvent::set_touch_minor(size_t index, float minor)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setTouchMinor(minor);
}

float MirMotionEvent::size(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getSize();
}

void MirMotionEvent::set_size(size_t index, float size)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setSize(size);
}

float MirMotionEvent::pressure(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getPressure();
}

void MirMotionEvent::set_pressure(size_t index, float pressure)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setPressure(pressure);
}

float MirMotionEvent::orientation(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getOrientation();
}

void MirMotionEvent::set_orientation(size_t index, float orientation)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setOrientation(orientation);
}

float MirMotionEvent::vscroll(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getVscroll();
}

void MirMotionEvent::set_vscroll(size_t index, float vscroll)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setVscroll(vscroll);
}

float MirMotionEvent::hscroll(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getHscroll();
}

void MirMotionEvent::set_hscroll(size_t index, float hscroll)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setHscroll(hscroll);
}

MirTouchTooltype MirMotionEvent::tool_type(size_t index) const
{
    throw_if_out_of_bounds(index);

    return static_cast<MirTouchTooltype>(event.asReader().getMotionSet().getMotions()[index].getToolType());
}

void MirMotionEvent::set_tool_type(size_t index, MirTouchTooltype tool_type)
{
    throw_if_out_of_bounds(index);

    auto capnp_tool_type = static_cast<mir::capnp::MotionEventSet::Motion::ToolType>(tool_type);
    event.getMotionSet().getMotions()[index].setToolType(capnp_tool_type);
}

int MirMotionEvent::action(size_t index) const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getMotionSet().getMotions()[index].getAction();
}

void MirMotionEvent::set_action(size_t index, int action)
{
    throw_if_out_of_bounds(index);

    event.getMotionSet().getMotions()[index].setAction(action);
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
