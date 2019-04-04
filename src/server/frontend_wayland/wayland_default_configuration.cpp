/*
 * Copyright © 2015-2019 Canonical Ltd.
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
#include "mir/frontend/wayland.h"

#include "wayland_connector.h"
#include "xdg_shell_v6.h"
#include "xdg_shell_stable.h"
#include "layer_shell_v1.h"
#include "xwayland_wm_shell.h"
#include "mir_display.h"
#include "wl_seat.h"

#include "mir/graphics/platform.h"
#include "mir/options/default_configuration.h"
#include "mir/scene/session.h"

namespace mf = mir::frontend;
namespace mo = mir::options;

namespace
{
auto const wl_shell       = "wl_shell";
auto const xdg_shell      = "xdg_wm_base";
auto const xdg_shell_v6   = "zxdg_shell_v6";
auto const layer_shell_v1 = "zwlr_layer_shell_v1";

auto configure_wayland_extensions(std::string extensions,
    bool x11_enabled,
    std::vector<mir::WaylandExtensionHook> const& wayland_extension_hooks)
    -> std::unique_ptr<mf::WaylandExtensions>
{
    struct WaylandExtensions : mf::WaylandExtensions
    {
        WaylandExtensions(
            std::set<std::string> const& extension,
            bool x11_enabled,
            std::vector<mir::WaylandExtensionHook> const& wayland_extension_hooks) :
            extension{extension}, x11_enabled{x11_enabled}, wayland_extension_hooks{wayland_extension_hooks} {}

    protected:
        virtual void custom_extensions(
            wl_display* display,
            std::shared_ptr<mf::Shell> const& shell,
            mf::WlSeat* seat,
            mf::OutputManager* const output_manager)
        {
            if (extension.find(wl_shell) != extension.end())
                add_extension(wl_shell, mf::create_wl_shell(display, shell, seat, output_manager));

            if (extension.find(xdg_shell_v6) != extension.end())
                add_extension(xdg_shell_v6, std::make_shared<mf::XdgShellV6>(display, shell, *seat, output_manager));

            if (extension.find(xdg_shell) != extension.end())
                add_extension(xdg_shell, std::make_shared<mf::XdgShellStable>(display, shell, *seat, output_manager));

            if (extension.find(layer_shell_v1) != extension.end())
                add_extension(layer_shell_v1, std::make_shared<mf::LayerShellV1>(display, shell, *seat,
                                                                                 output_manager));

            std::function<void(std::function<void()>&& work)> run_on_wayland_mainloop = [seat](std::function<void()>&& work)
                {
                    seat->spawn(std::move(work));
                };
            for (auto const& hook : wayland_extension_hooks)
            {
                if (extension.find(hook.name) != extension.end())
                    add_extension(hook.name, hook.builder(display, run_on_wayland_mainloop));
            }

            if (x11_enabled)
                add_extension("x11-support", std::make_shared<mf::XWaylandWMShell>(shell, *seat, output_manager));
        }

        std::set<std::string> const extension;
        const bool x11_enabled;
        std::vector<mir::WaylandExtensionHook> const wayland_extension_hooks;
    };

    std::set<std::string> extension;
    extensions += ':';

    for (char const* start = extensions.c_str(); char const* end = strchr(start, ':'); start = end+1)
    {
        if (start != end)
            extension.insert(std::string{start, end});
    }

    return std::make_unique<WaylandExtensions>(extension, x11_enabled, wayland_extension_hooks);
}
}

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            auto options = the_options();
            bool const arw_socket = options->is_set(options::arw_server_socket_opt);

            optional_value<std::string> display_name;

            if (options->is_set(options::wayland_socket_name_opt))
                display_name = options->get<std::string>(options::wayland_socket_name_opt);

            auto const wayland_extensions =
                options->get(mo::wayland_extensions_opt, mo::wayland_extensions_value);

            auto const display_config = std::make_shared<mf::MirDisplay>(
                the_frontend_display_changer(),
                the_display_configuration_observer_registrar());

            auto wayland_filter = [this](std::shared_ptr<frontend::Session> const& session, char const* protocol)
                {
                    return wayland_extension_filter(std::static_pointer_cast<scene::Session>(session), protocol);
                };

            return std::make_shared<mf::WaylandConnector>(
                display_name,
                the_frontend_shell(),
                display_config,
                the_input_device_hub(),
                the_seat(),
                the_buffer_allocator(),
                the_session_authorizer(),
                arw_socket,
                configure_wayland_extensions(wayland_extensions, options->is_set(mo::x11_display_opt), wayland_extension_hooks),
                wayland_filter);
        });
}

void mir::DefaultServerConfiguration::add_wayland_extension(
    std::string const& name,
    std::function<std::shared_ptr<void>(
        wl_display*,
        std::function<void(std::function<void()>&& work)> const& run_on_wayland_mainloop)> builder)
{
    wayland_extension_hooks.push_back({name, builder});
}

void mir::DefaultServerConfiguration::set_wayland_extension_filter(WaylandProtocolExtensionFilter const& extension_filter)
{
    wayland_extension_filter = extension_filter;
}

auto mir::frontend::get_window(wl_resource* window) -> std::shared_ptr<Surface>
{
    if (auto result = get_wl_shell_window(window))
        return result;

    if (auto result = XdgShellStable::get_window(window))
        return result;

    if (auto result = XdgShellV6::get_window(window))
        return result;

    return {};
}
