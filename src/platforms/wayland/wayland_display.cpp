/*
 * Copyright © Canonical Ltd.
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
 */

#include "wayland_display.h"

#include <mir/options/option.h>
#include <mir/fatal.h>
#include <boost/throw_exception.hpp>

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
char const* wayland_host_option_description{"Display name for host compositor, e.g. wayland-0."};
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
    {
        fatal_error("%s option required for Wayland platform", wayland_host_option_name);
    }

    auto const wayland_host = options.get<std::string>(wayland_host_option_name);
    static auto const wayland_display = std::make_unique<WaylandDisplay>(wayland_host.c_str());

    if (wayland_display->wl_display)
    {
        return wayland_display->wl_display;
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to connect to Wayland display '" + wayland_host + "'"));
    }
}

auto mir::platform::wayland::connection_options_supplied(mir::options::Option const& options) -> bool
{
    return options.is_set(wayland_host_option_name);
}
