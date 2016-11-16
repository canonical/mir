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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/events/pointer_event.h"

MirPointerEvent::MirPointerEvent()
{
    event.initInput();
    event.getInput().initPointer();
}

MirPointerEvent::MirPointerEvent(MirInputDeviceId dev,
                    std::chrono::nanoseconds et,
                    MirInputEventModifiers mods,
                    std::vector<uint8_t> const& cookie,
                    MirPointerAction action,
                    MirPointerButtons buttons,
                    float x,
                    float y,
                    float dx,
                    float dy,
                    float vscroll,
                    float hscroll)
    : MirInputEvent(dev, et, mods, cookie)
{
    auto input = event.getInput();
    input.initPointer();
    auto ptr = event.getInput().getPointer();
    ptr.setX(x);
    ptr.setY(y);
    ptr.setDx(dx);
    ptr.setDy(dy);
    ptr.setVscroll(vscroll);
    ptr.setHscroll(hscroll);
    ptr.setButtons(buttons);
    ptr.setAction(static_cast<mir::capnp::PointerEvent::PointerAction>(action));
}

MirPointerButtons MirPointerEvent::buttons() const
{
    return event.asReader().getInput().getPointer().getButtons();
}

void MirPointerEvent::set_buttons(MirPointerButtons buttons)
{
    event.getInput().getPointer().setButtons(buttons);
}

float MirPointerEvent::x() const
{
    return event.asReader().getInput().getPointer().getX();
}

void MirPointerEvent::set_x(float x)
{
    event.getInput().getPointer().setX(x);
}

float MirPointerEvent::y() const
{
    return event.asReader().getInput().getPointer().getY();
}

void MirPointerEvent::set_y(float y)
{
    event.getInput().getPointer().setY(y);
}

float MirPointerEvent::dx() const
{
    return event.asReader().getInput().getPointer().getDx();
}

void MirPointerEvent::set_dx(float dx)
{
    event.getInput().getPointer().setDx(dx);
}

float MirPointerEvent::dy() const
{
    return event.asReader().getInput().getPointer().getDy();
}

void MirPointerEvent::set_dy(float dy)
{
    event.getInput().getPointer().setDy(dy);
}

float MirPointerEvent::vscroll() const
{
    return event.asReader().getInput().getPointer().getVscroll();
}

void MirPointerEvent::set_vscroll(float vs)
{
    event.getInput().getPointer().setVscroll(vs);
}

float MirPointerEvent::hscroll() const
{
    return event.asReader().getInput().getPointer().getHscroll();
}

void MirPointerEvent::set_hscroll(float hs)
{
    event.getInput().getPointer().setHscroll(hs);
}

MirPointerAction MirPointerEvent::action() const
{
    return static_cast<MirPointerAction>(event.asReader().getInput().getPointer().getAction());
}

void MirPointerEvent::set_action(MirPointerAction action)
{
    event.getInput().getPointer().setAction(static_cast<mir::capnp::PointerEvent::PointerAction>(action));
}
