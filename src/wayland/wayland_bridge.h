/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_WAYLAND_WAYLAND_BRIDGE_H
#define MIR_WAYLAND_WAYLAND_BRIDGE_H

#include <cstdint>

struct wl_client;

namespace mir::wayland::cpp
{
class GlobalBoundCallback
{
public:
    virtual ~GlobalBoundCallback() = default;
    virtual void on_global_bound(uint32_t version, )
};
}

#endif