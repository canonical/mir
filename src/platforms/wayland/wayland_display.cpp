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

#include <mir/options/option.h>

namespace mpw = mir::platform::wayland;

namespace
{
struct WaylandDisplay
{
    struct wl_display* const wl_display;

    WaylandDisplay(char const* wayland_host) :
        wl_display{wl_display_connect(wayland_host)} {}

    ~WaylandDisplay() { if (wl_display) wl_display_disconnect(wl_display); }
};

char const* wayland_host_option_name{"wayland-host"};
char const* wayland_host_option_description{"Socket name for host compositor"};
}

void mpw::add_connection_options(boost::program_options::options_description& config)
{
    config.add_options()
        (wayland_host_option_name,
         boost::program_options::value<std::string>(),
         wayland_host_option_description);
}

auto mpw::connection(options::Option const& options) -> struct wl_display*
{
    if (!options.is_set(wayland_host_option_name))
        return nullptr;

    static auto const wayland_display =
        std::make_unique<WaylandDisplay>(options.get<std::string>(wayland_host_option_name).c_str());

    return wayland_display->wl_display;
}
