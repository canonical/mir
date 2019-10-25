/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_PLATFORM_WAYLAND_DISPLAY_H_
#define MIR_PLATFORM_WAYLAND_DISPLAY_H_

#include <wayland-client.h>

namespace mir
{
namespace platform
{
namespace wayland
{
auto connection() -> wl_display*;
}
}
}

#endif //MIR_PLATFORM_WAYLAND_DISPLAY_H_
