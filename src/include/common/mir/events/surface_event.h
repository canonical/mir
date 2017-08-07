/*
 * Copyright © 2016-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_COMMON_SURFACE_EVENT_H_
#define MIR_COMMON_SURFACE_EVENT_H_

#include "mir/events/event.h"

typedef struct MirBlob MirBlob;

struct MirSurfaceEvent : MirEvent
{
    MirSurfaceEvent();

    int id() const;
    void set_id(int id);

    MirWindowAttrib attrib() const;
    void set_attrib(MirWindowAttrib attrib);

    int value() const;
    void set_value(int value);

    void set_dnd_handle(std::vector<uint8_t> const& handle);
    MirBlob* dnd_handle() const;
};

#endif /* MIR_COMMON_SURFACE_EVENT_H_ */
