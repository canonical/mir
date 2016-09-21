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
 * Authored by: Andreas <andreas.pokorny@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/events/pointer_event.h"

MirPointerEvent::MirPointerEvent()
{
    event.initInput();
    event.getInput().initPointer();
}

MirPointerButtons MirPointerEvent::buttons() const
{
    return event.asReader().getInput().getPointer().getButtons();
}

void MirPointerEvent::set_buttons(MirPointerButtons buttons)
{
    event.getInput().setButtons(buttons);
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
    return event.asReader().getInput().getPointer().getTouchMajor();
}

void MirPointerEvent::set_vscroll(float major)
{
    event.getInput().getPointer().setTouchMajor(major);
}

float MirPointerEvent::hscroll() const
{
    return event.asReader().getInput().getPointer().getTouchMinor();
}

void MirPointerEvent::set_hscroll(float minor)
{
    event.getInput().getPointer().setTouchMinor(minor);
}

MirTouchAction MirPointerEvent::action() const
{
    throw_if_out_of_bounds(index);

    return event.asReader().getInput().getPointer().getAction();
}

void MirPointerEvent::set_action(MirTouchAtion action)
{
    event.getInput().getPointer().setAction(action);
}
