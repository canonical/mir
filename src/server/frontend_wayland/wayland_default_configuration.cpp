/*
 * Copyright © 2015, 2017 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/default_server_configuration.h"
#include "wayland_connector.h"
#include "xdg_shell_v6.h"
#include "xdg_shell_stable.h"
#include "xwayland_wm_shell.h"

#include "mir/frontend/display_changer.h"
#include "mir/graphics/platform.h"
#include "mir/options/default_configuration.h"

namespace mf = mir::frontend;
namespace mo = mir::options;

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    struct WaylandExtensions : mf::WaylandExtensions
    {
        WaylandExtensions(bool x11_enabled, bool xdg_stable_enabled)
            : x11_enabled{x11_enabled},
              xdg_stable_enabled{xdg_stable_enabled}
        {}
    protected:
        virtual void custom_extensions(
            wl_display* display,
            std::shared_ptr<mf::Shell> const& shell,
            mf::WlSeat* seat,
            mf::OutputManager* const output_manager)
        {
            if (!getenv("MIR_DISABLE_XDG_SHELL_V6_UNSTABLE"))
                add_extension("zxdg_shell_v6", std::make_shared<mf::XdgShellV6>(display, shell, *seat, output_manager));

            if (x11_enabled)
                add_extension("x11-support", std::make_shared<mf::XWaylandWMShell>(shell, *seat, output_manager));

            if (xdg_stable_enabled)
                add_extension("xdg_shell_stable", std::make_shared<mf::XdgShellStable>(display, shell, *seat, output_manager));
        }
        const bool x11_enabled;
        const bool xdg_stable_enabled;
    };

    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            auto options = the_options();
            bool const arw_socket = options->is_set(options::arw_server_socket_opt);

            optional_value<std::string> display_name;

            if (options->is_set(options::wayland_socket_name_opt))
                display_name = options->get<std::string>(options::wayland_socket_name_opt);

            return std::make_shared<mf::WaylandConnector>(
                display_name,
                the_frontend_shell(),
                *the_frontend_display_changer(),
                the_input_device_hub(),
                the_seat(),
                the_buffer_allocator(),
                the_session_authorizer(),
                arw_socket,
                std::make_unique<WaylandExtensions>(
                    options->is_set(mo::x11_display_opt),
                    true));
        });
}
