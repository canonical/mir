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

#ifndef MIR_COMMON_RESIZE_EVENT_H_
#define MIR_COMMON_RESIZE_EVENT_H_

#include "mir/events/event.h"

struct MirResizeEvent : MirEvent
{
    MirResizeEvent();

    int surface_id() const;
    void set_surface_id(int id);

    int width() const;
    void set_width(int width);

    int height() const;
    void set_height(int height);
};

#endif /* MIR_COMMON_RESIZE_EVENT_H_ */
