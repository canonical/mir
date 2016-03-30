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
#include <limits>

#include "mir/events/event.h"

struct MirKeymapEvent : MirEvent
{
    MirKeymapEvent();

    int surface_id() const;
    void set_surface_id(int id);

    MirInputDeviceId device_id() const;
    void set_device_id(MirInputDeviceId id);

    char const* buffer() const;
    /* FIXME We takes over ownership of the buffer pointer, but we should not be.
       Either user a unique_ptr here later (when we dont depend on is_trivially_copyable)
       or copy it into a vector or do a deep copy.
     */
    void set_buffer(char const* buffer);
    void free_buffer();

    size_t size() const;
    void set_size(size_t size);

    /* FIXME Need to handle the special case of Keymap events due to char const* buffer */
    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);
    MirEvent* clone() const;

private:
    int surface_id_{-1};
    MirInputDeviceId device_id_{std::numeric_limits<MirInputDeviceId>::max()};
    char const* buffer_{nullptr};
    size_t size_{0};
};

#endif /* MIR_COMMON_KEYMAP_EVENT_H_ */
