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

#include "wayland_display.h"

namespace mpw = mir::platform::wayland;

namespace
{
struct WaylandDisplay
{
    struct wl_display* const wl_display = wl_display_connect(getenv("WAYLAND_DISPLAY"));

    ~WaylandDisplay() { if (wl_display) wl_display_disconnect(wl_display); }
} wayland_display;
}

auto mpw::connection() -> struct wl_display*
{
    return wayland_display.wl_display;
}
