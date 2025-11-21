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

#include <mir/events/close_window_event.h>

MirCloseWindowEvent::MirCloseWindowEvent() :
    MirEvent{mir_event_type_close_window}
{
}

auto MirCloseWindowEvent::clone() const -> MirCloseWindowEvent*
{
    return new MirCloseWindowEvent{*this};
}

int MirCloseWindowEvent::surface_id() const
{
    return surface_id_;
}

void MirCloseWindowEvent::set_surface_id(int id)
{
    surface_id_ = id;
}
