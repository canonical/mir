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

#ifndef MIR_COMMON_SURFACE_EVENT_H_
#define MIR_COMMON_SURFACE_EVENT_H_

#include "mir/events/event.h"

struct MirSurfaceEvent : MirEvent
{
    MirSurfaceEvent();

    int id() const;
    void set_id(int id);

    MirSurfaceAttrib attrib() const;
    void set_attrib(MirSurfaceAttrib attrib);

    int value() const;
    void set_value(int value);

private:
    int id_{0};
    MirSurfaceAttrib attrib_;
    int value_{0};
};

#endif /* MIR_COMMON_SURFACE_EVENT_H_ */
