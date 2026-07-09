/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_FRONTEND_WAYLAND_GLOBAL_FACTORY_H_
#define MIR_FRONTEND_WAYLAND_GLOBAL_FACTORY_H_

#include "global_factory.h"
#include "input_trigger_registry.h"

#include <mir/graphics/drm_formats.h>

#include <functional>
#include <memory>
#include <vector>

namespace mir
{
class Executor;
class DecorationStrategy;

namespace shell
{
class Shell;
}

namespace compositor
{
class ScreenShooterFactory;
}

namespace scene
{
class Session;
class IdleHub;
}

namespace graphics
{
class GraphicBufferAllocator;
class DRMRenderingProvider;
}

namespace input
{
class CursorObserverMultiplexer;
}

namespace time
{
class Clock;
}

namespace wayland_rs
{
class WaylandClientRegistry;
class WaylandServer;
}

namespace frontend
{
class KeyboardStateTracker;
class SurfaceStack;

/// The per-client visibility predicate: given the client's session and an
/// interface name, decide whether the client may see (bind) that global.
///
/// This is the wayland_rs equivalent of the old
/// `WaylandConnector::WaylandProtocolExtensionFilter`.
using WaylandProtocolExtensionFilter =
    std::function<bool(std::shared_ptr<scene::Session> const& session, char const* protocol)>;

/// Concrete `wayland_rs::GlobalFactory` for the Mir frontend.
///
/// The Rust server calls into this to (a) construct the C++ implementation of
/// each protocol object a client binds (`create_*`) and (b) decide per-client
/// global visibility (`can_view`). Each `create_*` resolves the raw client to
/// its registered `WaylandClient` and constructs the matching protocol
/// implementation.
///
/// NOTE: the individual `create_*` implementations are filled in as each
/// protocol is migrated (Phase B). Until then they throw.
class WaylandGlobalFactory : public wayland_rs::GlobalFactory
{
public:
    WaylandGlobalFactory(
        wayland_rs::WaylandClientRegistry& registry,
        WaylandProtocolExtensionFilter extension_filter,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<Executor> frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> allocator,
        wayland_rs::WaylandServer& server,
        std::shared_ptr<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const> drm_providers,
        std::shared_ptr<scene::IdleHub> idle_hub,
        std::shared_ptr<DecorationStrategy> decoration_strategy,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<InputTriggerRegistry::ActionGroupManager> action_group_manager,
        std::shared_ptr<InputTriggerRegistry> input_trigger_registry,
        std::shared_ptr<KeyboardStateTracker> keyboard_state_tracker,
        std::shared_ptr<std::vector<graphics::DRMFormat> const> shm_formats,
        std::shared_ptr<compositor::ScreenShooterFactory> screen_shooter_factory,
        std::shared_ptr<SurfaceStack> surface_stack,
        std::shared_ptr<input::CursorObserverMultiplexer> cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> clock);

    auto create_ext_data_control_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtDataControlManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtDataControlManagerV1> override;
    auto create_ext_data_control_offer_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtDataControlOfferV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtDataControlOfferV1> override;
    auto create_ext_foreign_toplevel_list_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtForeignToplevelListV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtForeignToplevelListV1> override;
    auto create_ext_foreign_toplevel_handle_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtForeignToplevelHandleV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtForeignToplevelHandleV1> override;
    auto create_ext_output_image_capture_source_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtOutputImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtOutputImageCaptureSourceManagerV1> override;
    auto create_ext_foreign_toplevel_image_capture_source_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtForeignToplevelImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtForeignToplevelImageCaptureSourceManagerV1> override;
    auto create_ext_image_copy_capture_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtImageCopyCaptureManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtImageCopyCaptureManagerV1> override;
    auto create_ext_input_trigger_action_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtInputTriggerActionManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtInputTriggerActionManagerV1> override;
    auto create_ext_input_trigger_registration_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtInputTriggerRegistrationManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtInputTriggerRegistrationManagerV1> override;
    auto create_ext_session_lock_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtSessionLockManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtSessionLockManagerV1> override;
    auto create_wp_fractional_scale_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::FractionalScaleManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::FractionalScaleManagerV1> override;
    auto create_zwp_idle_inhibit_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::IdleInhibitManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::IdleInhibitManagerV1> override;
    auto create_zwp_input_method_context_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputMethodContextV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputMethodContextV1> override;
    auto create_zwp_input_method_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputMethodV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputMethodV1> override;
    auto create_zwp_input_panel_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputPanelV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputPanelV1> override;
    auto create_zwp_input_method_manager_v2(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputMethodManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputMethodManagerV2> override;
    auto create_zwp_linux_dmabuf_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::LinuxDmabufV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::LinuxDmabufV1> override;
    auto create_wp_linux_drm_syncobj_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::LinuxDrmSyncobjManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::LinuxDrmSyncobjManagerV1> override;
    auto create_mir_shell_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::MirShellV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::MirShellV1> override;
    auto create_zwp_pointer_constraints_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::PointerConstraintsV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::PointerConstraintsV1> override;
    auto create_zwp_primary_selection_device_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::PrimarySelectionDeviceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::PrimarySelectionDeviceManagerV1> override;
    auto create_zwp_primary_selection_offer_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::PrimarySelectionOfferV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::PrimarySelectionOfferV1> override;
    auto create_zwp_relative_pointer_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::RelativePointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::RelativePointerManagerV1> override;
    auto create_org_kde_kwin_server_decoration_manager(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ServerDecorationManagerMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ServerDecorationManager> override;
    auto create_zwp_text_input_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV1> override;
    auto create_zwp_text_input_manager_v2(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV2> override;
    auto create_zwp_text_input_manager_v3(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV3Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV3> override;
    auto create_wp_viewporter(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ViewporterMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Viewporter> override;
    auto create_zwp_virtual_keyboard_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::VirtualKeyboardManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::VirtualKeyboardManagerV1> override;
    auto create_wl_compositor(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::CompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Compositor> override;
    auto create_wl_shm(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ShmMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Shm> override;
    auto create_wl_data_offer(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::DataOfferMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::DataOffer> override;
    auto create_wl_data_device_manager(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::DataDeviceManagerMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::DataDeviceManager> override;
    auto create_wl_shell(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ShellMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Shell> override;
    auto create_wl_seat(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::SeatMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Seat> override;
    auto create_wl_output(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::OutputMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Output> override;
    auto create_wl_subcompositor(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::SubcompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Subcompositor> override;
    auto create_zwlr_foreign_toplevel_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ForeignToplevelManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ForeignToplevelManagerV1> override;
    auto create_zwlr_foreign_toplevel_handle_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ForeignToplevelHandleV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ForeignToplevelHandleV1> override;
    auto create_zwlr_layer_shell_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::LayerShellV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::LayerShellV1> override;
    auto create_zwlr_screencopy_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ScreencopyManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ScreencopyManagerV1> override;
    auto create_zwlr_virtual_pointer_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::VirtualPointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::VirtualPointerManagerV1> override;
    auto create_xdg_activation_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgActivationV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgActivationV1> override;
    auto create_zxdg_decoration_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgDecorationManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgDecorationManagerV1> override;
    auto create_zxdg_output_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgOutputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgOutputManagerV1> override;
    auto create_zxdg_shell_v6(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgShellV6Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgShellV6> override;
    auto create_xdg_wm_base(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgWmBaseMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgWmBase> override;
    auto can_view(rust::Str interface_name, rust::Box<wayland_rs::WaylandClientId> client_id) -> bool override;

private:
    wayland_rs::WaylandClientRegistry& registry;
    WaylandProtocolExtensionFilter const extension_filter;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<Executor> const frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    wayland_rs::WaylandServer& server;
    std::shared_ptr<std::vector<std::shared_ptr<graphics::DRMRenderingProvider>> const> const drm_providers;
    std::shared_ptr<scene::IdleHub> const idle_hub;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> const action_group_manager;
    std::shared_ptr<InputTriggerRegistry> const input_trigger_registry;
    std::shared_ptr<KeyboardStateTracker> const keyboard_state_tracker;
    std::shared_ptr<std::vector<graphics::DRMFormat> const> const shm_formats;
    std::shared_ptr<compositor::ScreenShooterFactory> const screen_shooter_factory;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_GLOBAL_FACTORY_H_
