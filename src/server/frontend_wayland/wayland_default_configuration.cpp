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

#include <mir/default_server_configuration.h>
#include <mir/frontend/wayland.h>
#include <mir/options/default_configuration.h>
#include <mir/scene/session.h>

#include "wayland_connector.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;


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
        "xdg_activation_v1",
        "wp_fractional_scale_manager_v1"};
}

auto mf::get_supported_extensions() -> std::vector<std::string>
{
    return std::vector<std::string>{
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
        "ext_image_copy_capture_manager_v1",
        "zwp_primary_selection_device_manager_v1",
        "ext_session_lock_manager_v1",
        "mir_shell_v1",
        "zxdg_decoration_manager_v1",
        "wp_fractional_scale_manager_v1",
        "xdg_activation_v1",
        "ext_data_control_manager_v1",
        "ext_input_trigger_registration_manager_v1",
        "ext_input_trigger_action_manager_v1"};
}

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            auto options = the_options();
            bool const arw_socket = options->is_set(options::arw_server_socket_opt);

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
                std::make_unique<mf::WaylandExtensions>(),
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

auto mir::frontend::get_window(wl_resource* surface) -> std::shared_ptr<ms::Surface>
{
    // TODO: get_window() needs reworking for the Rust-based Wayland architecture.
    // The internal APIs (get_wl_shell_window, XdgShellStable::get_window, LayerShellV1::get_window)
    // no longer accept wl_resource* — they use typed Impl pointers instead.
    (void)surface;
    return {};
}
