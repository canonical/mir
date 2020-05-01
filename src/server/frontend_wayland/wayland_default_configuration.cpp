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
#include "xdg_output_v1.h"
#include "layer_shell_v1.h"
#include "xwayland_wm_shell.h"
#include "mir_display.h"
#include "wl_seat.h"
#include "xdg-output-unstable-v1_wrapper.h"

#include "mir/graphics/platform.h"
#include "mir/options/default_configuration.h"
#include "mir/scene/session.h"
#include "mir/log.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mo = mir::options;
namespace mw = mir::wayland;

namespace
{
struct ExtensionBuilder
{
    std::string name;
    std::function<std::shared_ptr<void>(mf::WaylandExtensions::Context const& ctx)> build;
};

std::string const x11_support_extension_name = "x11-support";

/// Extensions that are not in the set returned by mf::get_standard_extensions() should generally be listed in
/// include/miral/miral/wayland_extensions.h for easy access by shells.
std::vector<ExtensionBuilder> const internal_builders = {
    {
        mw::Shell::interface_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return mf::create_wl_shell(ctx.display, ctx.shell, ctx.seat, ctx.output_manager); }
    },
    {
        mw::XdgShellV6::interface_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return std::make_shared<mf::XdgShellV6>(ctx.display, ctx.shell, *ctx.seat, ctx.output_manager); }
    },
    {
        mw::XdgWmBase::interface_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return std::make_shared<mf::XdgShellStable>(ctx.display, ctx.shell, *ctx.seat, ctx.output_manager); }
    },
    {
        mw::LayerShellV1::interface_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return std::make_shared<mf::LayerShellV1>(ctx.display, ctx.shell, *ctx.seat, ctx.output_manager); }
    },
    {
        mw::XdgOutputManagerV1::interface_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return create_xdg_output_manager_v1(ctx.display, ctx.output_manager); }
    },
    {
        x11_support_extension_name, [](auto const& ctx) -> std::shared_ptr<void>
            { return std::make_shared<mf::XWaylandWMShell>(ctx.shell, *ctx.seat, ctx.output_manager); }
    },
};

struct WaylandExtensions : mf::WaylandExtensions
{
    WaylandExtensions(
        std::vector<ExtensionBuilder> enabled_internal_builders,
        std::vector<mir::WaylandExtensionHook> enabled_external_hooks) :
        enabled_internal_builders{move(enabled_internal_builders)},
        enabled_external_hooks{move(enabled_external_hooks)}
    {
    }

protected:
    void custom_extensions(Context const& context) override
    {
        for (auto const& extension : enabled_internal_builders)
        {
            add_extension(extension.name, extension.build(context));
        }
    }

    void run_builders(
        wl_display* display,
        std::function<void(std::function<void()>&& work)> const& run_on_wayland_mainloop) override
    {
        for (auto const& hook : enabled_external_hooks)
        {
            add_extension(hook.name, hook.builder(display, run_on_wayland_mainloop));
        }
    }

    std::vector<ExtensionBuilder> const enabled_internal_builders;
    std::vector<mir::WaylandExtensionHook> const enabled_external_hooks;
};

auto configure_wayland_extensions(
    std::set<std::string> const& enabled_extension_names,
    bool x11_enabled,
    std::vector<mir::WaylandExtensionHook> const& external_extension_hooks)
    -> std::unique_ptr<mf::WaylandExtensions>
{
    auto remaining_extension_names = enabled_extension_names;

    std::vector<ExtensionBuilder> enabled_internal_builders;
    for (auto const& builder : internal_builders)
    {
        if (enabled_extension_names.find(builder.name) != enabled_extension_names.end())
        {
            remaining_extension_names.erase(builder.name);
            enabled_internal_builders.push_back(builder);
        }
        else if (builder.name == x11_support_extension_name && x11_enabled)
        {
            enabled_internal_builders.push_back(builder);
        }
    }

    std::vector<mir::WaylandExtensionHook> enabled_external_hooks;
    for (auto const& hook : external_extension_hooks)
    {
        if (enabled_extension_names.find(hook.name) != enabled_extension_names.end())
        {
            remaining_extension_names.erase(hook.name);
            enabled_external_hooks.push_back(hook);
        }
    }

    for (auto const& name : remaining_extension_names)
    {
        mir::log_warning("Wayland extension %s not supported", name.c_str());
    }

    return std::make_unique<WaylandExtensions>(move(enabled_internal_builders), move(enabled_external_hooks));
}
}

auto mf::get_standard_extensions() -> std::vector<std::string>
{
    return std::vector<std::string>{
        mw::Shell::interface_name,
        mw::XdgWmBase::interface_name,
        mw::XdgShellV6::interface_name};
}

auto mf::get_supported_extensions() -> std::vector<std::string>
{
    std::vector<std::string> result;
    for (auto const& builder : internal_builders)
    {
        if (builder.name != x11_support_extension_name)
        {
            result.push_back(builder.name);
        }
    }
    return result;
}

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            auto options = the_options();
            bool const arw_socket = options->is_set(options::arw_server_socket_opt);

            auto wayland_extensions = std::set<std::string>{
                enabled_wayland_extensions.begin(),
                enabled_wayland_extensions.end()};

            auto const display_config = std::make_shared<mf::MirDisplay>(
                the_frontend_display_changer(),
                the_display_configuration_observer_registrar());

            return std::make_shared<mf::WaylandConnector>(
                the_shell(),
                display_config,
                the_input_device_hub(),
                the_seat(),
                the_buffer_allocator(),
                the_session_authorizer(),
                arw_socket,
                configure_wayland_extensions(
                    wayland_extensions,
                    options->is_set(mo::x11_display_opt),
                    wayland_extension_hooks),
                wayland_extension_filter);
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

void mir::DefaultServerConfiguration::set_enabled_wayland_extensions(std::vector<std::string> const& extensions)
{
    enabled_wayland_extensions = extensions;
}

auto mir::frontend::get_window(wl_resource* surface) -> std::shared_ptr<ms::Surface>
{
    if (auto result = get_wl_shell_window(surface))
        return result;

    if (auto result = XdgShellStable::get_window(surface))
        return result;

    if (auto result = XdgShellV6::get_window(surface))
        return result;

    if (auto result = LayerShellV1::get_window(surface))
        return result;

    return {};
}
