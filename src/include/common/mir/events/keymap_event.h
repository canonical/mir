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

#ifndef MIR_COMMON_KEYMAP_EVENT_H_
#define MIR_COMMON_KEYMAP_EVENT_H_

#include <xkbcommon/xkbcommon.h>

#include "mir/events/event.h"

struct MirKeymapEvent : MirEvent
{
    MirKeymapEvent();

    int surface_id() const;
    void set_surface_id(int id);

    MirInputDeviceId device_id() const;
    void set_device_id(MirInputDeviceId id);

    char const* buffer() const;
    void set_buffer(char const* buffer);

    size_t size() const;
};

#endif /* MIR_COMMON_KEYMAP_EVENT_H_ */
