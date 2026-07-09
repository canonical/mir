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

#include <ranges>

#include <mir/default_server_configuration.h>

#include <mir/frontend/wayland.h>
#include <mir/graphics/platform.h>
#include <mir/log.h>
#include <mir/options/default_configuration.h>
#include <mir/scene/session.h>

#include "wayland_connector.h"
#include "xwayland_wm_shell.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mo = mir::options;

namespace
{
/// The set of Wayland extensions the compositor is able to advertise. Globals
/// are now created unconditionally by `WaylandGlobalFactory`; per-client
/// visibility is gated by the `WaylandProtocolExtensionFilter` (via
/// `GlobalFactory::can_view`). This list is used to validate the configured
/// extension names and to answer `mf::get_supported_extensions()`.
std::vector<std::string> const supported_extension_names = {
    "wl_shell",
    "xdg_wm_base",
    "zwlr_layer_shell_v1",
    "zxdg_output_manager_v1",
    "zwlr_foreign_toplevel_manager_v1",
    "ext_foreign_toplevel_list_v1",
    "zwp_relative_pointer_manager_v1",
    "zwp_pointer_constraints_v1",
    "zwp_virtual_keyboard_manager_v1",
    "zwlr_virtual_pointer_manager_v1",
    "zwp_text_input_manager_v1",
    "zwp_text_input_manager_v2",
    "zwp_text_input_manager_v3",
    "zwp_input_method_v1",
    "zwp_input_panel_v1",
    "zwp_input_method_manager_v2",
    "zwp_idle_inhibit_manager_v1",
    "zwlr_screencopy_manager_v1",
    "ext_output_image_capture_source_manager_v1",
    "ext_foreign_toplevel_image_capture_source_manager_v1",
    "ext_image_copy_capture_manager_v1",
    "zwp_primary_selection_device_manager_v1",
    "ext_session_lock_manager_v1",
    "mir_shell_v1",
    "zxdg_decoration_manager_v1",
    "xdg_wm_dialog_v1",
    "org_kde_kwin_server_decoration_manager",
    "wp_fractional_scale_manager_v1",
    "xdg_activation_v1",
    "ext_data_control_manager_v1",
    "ext_input_trigger_registration_manager_v1",
    "ext_input_trigger_action_manager_v1",
};

struct ExtensionBuilder
{
    std::string global_name;
    std::function<std::shared_ptr<void>(mf::WaylandExtensions::Context const& ctx)> build;
};

ExtensionBuilder const xwayland_builder {
    "x11-support", [](auto const& ctx) -> std::shared_ptr<void>
        {
            return std::make_shared<mf::XWaylandWMShell>(
                ctx.wayland_executor,
                ctx.shell,
                ctx.session_authorizer,
                ctx.main_clipboard,
                *ctx.seat,
                ctx.surface_stack,
                ctx.surface_registry);
        }
};

struct WaylandExtensions : mf::WaylandExtensions
{
    WaylandExtensions(
        std::vector<ExtensionBuilder> enabled_internal_builders,
        std::vector<mir::WaylandExtensionHook> enabled_external_hooks) :
        enabled_internal_builders{std::move(enabled_internal_builders)},
        enabled_external_hooks{std::move(enabled_external_hooks)}
    {
    }

protected:
    void custom_extensions(Context const& context) override
    {
        for (auto const& extension : enabled_internal_builders)
        {
            add_extension(extension.global_name, extension.build(context));
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

    // Internal protocol globals are now created by WaylandGlobalFactory; only
    // extensions with a bespoke builder (currently just XWayland) are wired here.
    for (auto const& name : supported_extension_names)
    {
        remaining_extension_names.erase(name);
    }

    std::vector<ExtensionBuilder> enabled_internal_builders;
    if (x11_enabled)
    {
        enabled_internal_builders.push_back(xwayland_builder);
    }
    remaining_extension_names.erase(xwayland_builder.global_name);

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

    return std::make_unique<WaylandExtensions>(std::move(enabled_internal_builders), std::move(enabled_external_hooks));
}
}

auto mf::get_standard_extensions() -> std::vector<std::string>
{
    return std::vector<std::string>{
        "wl_shell",
        "xdg_wm_base",
        "zxdg_output_manager_v1",
        "zwp_text_input_manager_v1",
        "zwp_text_input_manager_v2",
        "zwp_text_input_manager_v3",
        "mir_shell_v1",
        "zxdg_decoration_manager_v1",
        "xdg_wm_dialog_v1",
        "xdg_activation_v1",
        "wp_fractional_scale_manager_v1"};
}

auto mf::get_supported_extensions() -> std::vector<std::string>
{
    return supported_extension_names;
}

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            auto options = the_options();
            bool const arw_socket = options->is_set(options::arw_server_socket_opt);

            auto const extension_keys = wayland_extension_policy_map | std::views::keys;
            std::set<std::string> const wayland_extensions(std::ranges::begin(extension_keys), std::ranges::end(extension_keys));

            auto const x11_enabled = options->is_set(mo::x11_display_opt) && options->get<bool>(mo::x11_display_opt);

            return std::make_shared<mf::WaylandConnector>(
                the_shell(),
                the_clock(),
                the_input_device_hub(),
                the_seat(),
                the_keyboard_observer_registrar(),
                the_input_device_registry(),
                the_composite_event_filter(),
                the_drag_icon_controller(),
                the_pointer_input_dispatcher(),
                the_buffer_allocator(),
                the_session_authorizer(),
                the_frontend_surface_stack(),
                the_display_configuration_observer_registrar(),
                the_main_clipboard(),
                the_primary_selection_clipboard(),
                the_text_input_hub(),
                the_idle_hub(),
                the_screen_shooter_factory(),
                the_main_loop(),
                arw_socket,
                configure_wayland_extensions(
                    wayland_extensions,
                    x11_enabled,
                    wayland_extension_hooks),
                wayland_extension_filter,
                the_accessibility_manager(),
                the_session_lock(),
                the_decoration_strategy(),
                the_session_coordinator(),
                the_token_authority(),
                the_rendering_platforms(),
                the_cursor_observer_multiplexer());
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

void mir::DefaultServerConfiguration::set_wayland_extension_policy(
    std::string const& interface_name,
    WaylandProtocolExtensionFilter const& policy)
{
    wayland_extension_policy_map[interface_name] = policy;
}

auto mir::frontend::get_window(wl_resource* /*surface*/) -> std::shared_ptr<ms::Surface>
{
    // TODO: the libwayland wl_resource-based MirAL window lookup bridge is not
    // available in the wayland_rs frontend. XdgShellStable/WlShell/LayerShellV1
    // now expose Weak<>-based get_window overloads; the MirAL bridge needs
    // porting once XWayland/MirAL are moved onto the new frontend.
    return {};
}
