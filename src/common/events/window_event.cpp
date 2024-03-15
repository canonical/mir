/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/events/window_event.h"

MirWindowEvent::MirWindowEvent() : MirEvent{mir_event_type_window}
{
}

auto MirWindowEvent::clone() const -> MirWindowEvent*
{
    return new MirWindowEvent{*this};
}

int MirWindowEvent::id() const
{
    return id_;
}

void MirWindowEvent::set_id(int id)
{
    id_ = id;
}

MirWindowAttrib MirWindowEvent::attrib() const
{
    return attrib_;
}

void MirWindowEvent::set_attrib(MirWindowAttrib attrib)
{
    attrib_ = attrib;
}

int MirWindowEvent::value() const
{
    return value_;
}

void MirWindowEvent::set_value(int value)
{
    value_ = value;
}
