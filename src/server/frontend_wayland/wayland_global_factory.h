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
class MainLoop;
template<typename T>
class ObserverRegistrar;

namespace shell
{
class AccessibilityManager;
class Shell;
class TokenAuthority;
}

namespace compositor
{
class ScreenShooterFactory;
}

namespace scene
{
class Session;
class IdleHub;
class SessionLock;
class SessionCoordinator;
class Clipboard;
class TextInputHub;
}

namespace graphics
{
class GraphicBufferAllocator;
class DRMRenderingProvider;
}

namespace input
{
class CursorObserverMultiplexer;
class InputDeviceHub;
class InputDeviceRegistry;
class KeyboardObserver;
class Seat;
class CompositeEventFilter;
}

namespace time
{
class Clock;
}

namespace wayland
{
class WaylandClientRegistry;
class WaylandServer;
}

namespace frontend
{
class KeyboardStateTracker;
class SurfaceStack;
class SurfaceRegistry;
class DesktopFileManager;
class ForeignToplevelIdentifierMap;
class WlSeat;
class OutputManager;
class SessionLockState;
class XdgActivationV1State;
class DragIconController;
class PointerInputDispatcher;

/// The per-client visibility predicate: given the client's session and an
/// interface name, decide whether the client may see (bind) that global.
///
/// This is the wayland_rs equivalent of the old
/// `WaylandConnector::WaylandProtocolExtensionFilter`.
using WaylandProtocolExtensionFilter =
    std::function<bool(std::shared_ptr<scene::Session> const& session, char const* protocol)>;

/// Concrete `wayland::GlobalFactory` for the Mir frontend.
///
/// The Rust server calls into this to (a) construct the C++ implementation of
/// each protocol object a client binds (`create_*`) and (b) decide per-client
/// global visibility (`can_view`). Each `create_*` resolves the raw client to
/// its registered `WaylandClient` and constructs the matching protocol
/// implementation.
///
/// NOTE: the individual `create_*` implementations are filled in as each
/// protocol is migrated (Phase B). Until then they throw.
class WaylandGlobalFactory : public wayland::GlobalFactory
{
public:
    WaylandGlobalFactory(
        wayland::WaylandClientRegistry& registry,
        WaylandProtocolExtensionFilter extension_filter,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<Executor> frame_callback_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> allocator,
        wayland::WaylandServer& server,
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
        std::shared_ptr<scene::SessionLock> session_lock,
        std::shared_ptr<input::CursorObserverMultiplexer> cursor_observer_multiplexer,
        std::shared_ptr<time::Clock> clock,
        std::shared_ptr<input::InputDeviceHub> input_hub,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> keyboard_observer_registrar,
        std::shared_ptr<input::Seat> mir_seat,
        std::shared_ptr<shell::AccessibilityManager> accessibility_manager,
        std::shared_ptr<SurfaceRegistry> surface_registry,
        std::shared_ptr<DesktopFileManager> desktop_file_manager,
        std::shared_ptr<input::InputDeviceRegistry> input_device_registry,
        std::shared_ptr<scene::SessionCoordinator> session_coordinator,
        std::shared_ptr<MainLoop> main_loop,
        std::shared_ptr<shell::TokenAuthority> token_authority,
        std::shared_ptr<scene::Clipboard> clipboard,
        std::shared_ptr<scene::Clipboard> primary_selection_clipboard,
        std::shared_ptr<DragIconController> drag_icon_controller,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
        std::shared_ptr<scene::TextInputHub> text_input_hub,
        std::shared_ptr<input::CompositeEventFilter> event_filter,
        OutputManager& output_manager);

    auto create_ext_data_control_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtDataControlManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtDataControlManagerV1> override;
    auto create_ext_data_control_offer_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtDataControlOfferV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtDataControlOfferV1> override;
    auto create_ext_foreign_toplevel_list_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtForeignToplevelListV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtForeignToplevelListV1> override;
    auto create_ext_foreign_toplevel_handle_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtForeignToplevelHandleV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtForeignToplevelHandleV1> override;
    auto create_ext_output_image_capture_source_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtOutputImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtOutputImageCaptureSourceManagerV1> override;
    auto create_ext_foreign_toplevel_image_capture_source_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtForeignToplevelImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtForeignToplevelImageCaptureSourceManagerV1> override;
    auto create_ext_image_copy_capture_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtImageCopyCaptureManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtImageCopyCaptureManagerV1> override;
    auto create_ext_input_trigger_action_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtInputTriggerActionManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtInputTriggerActionManagerV1> override;
    auto create_ext_input_trigger_registration_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtInputTriggerRegistrationManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtInputTriggerRegistrationManagerV1> override;
    auto create_ext_session_lock_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ExtSessionLockManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ExtSessionLockManagerV1> override;
    auto create_wp_fractional_scale_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::FractionalScaleManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::FractionalScaleManagerV1> override;
    auto create_zwp_idle_inhibit_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::IdleInhibitManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::IdleInhibitManagerV1> override;
    auto create_zwp_input_method_context_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::InputMethodContextV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::InputMethodContextV1> override;
    auto create_zwp_input_method_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::InputMethodV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::InputMethodV1> override;
    auto create_zwp_input_panel_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::InputPanelV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::InputPanelV1> override;
    auto create_zwp_input_method_manager_v2(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::InputMethodManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::InputMethodManagerV2> override;
    auto create_zwp_linux_dmabuf_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::LinuxDmabufV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::LinuxDmabufV1> override;
    auto create_wp_linux_drm_syncobj_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::LinuxDrmSyncobjManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::LinuxDrmSyncobjManagerV1> override;
    auto create_mir_shell_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::MirShellV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::MirShellV1> override;
    auto create_zwp_pointer_constraints_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::PointerConstraintsV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::PointerConstraintsV1> override;
    auto create_zwp_primary_selection_device_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::PrimarySelectionDeviceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::PrimarySelectionDeviceManagerV1> override;
    auto create_zwp_primary_selection_offer_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::PrimarySelectionOfferV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::PrimarySelectionOfferV1> override;
    auto create_zwp_relative_pointer_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::RelativePointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::RelativePointerManagerV1> override;
    auto create_org_kde_kwin_server_decoration_manager(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ServerDecorationManagerMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ServerDecorationManager> override;
    auto create_zwp_text_input_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::TextInputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::TextInputManagerV1> override;
    auto create_zwp_text_input_manager_v2(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::TextInputManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::TextInputManagerV2> override;
    auto create_zwp_text_input_manager_v3(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::TextInputManagerV3Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::TextInputManagerV3> override;
    auto create_wp_viewporter(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ViewporterMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Viewporter> override;
    auto create_zwp_virtual_keyboard_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::VirtualKeyboardManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::VirtualKeyboardManagerV1> override;
    auto create_wl_compositor(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::CompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Compositor> override;
    auto create_wl_shm(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ShmMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Shm> override;
    auto create_wl_data_offer(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::DataOfferMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::DataOffer> override;
    auto create_wl_data_device_manager(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::DataDeviceManagerMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::DataDeviceManager> override;
    auto create_wl_shell(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ShellMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Shell> override;
    auto create_wl_seat(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::SeatMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Seat> override;
    auto create_wl_output(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::OutputMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Output> override;
    auto create_wl_subcompositor(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::SubcompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::Subcompositor> override;
    auto create_zwlr_foreign_toplevel_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ForeignToplevelManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ForeignToplevelManagerV1> override;
    auto create_zwlr_foreign_toplevel_handle_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ForeignToplevelHandleV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ForeignToplevelHandleV1> override;
    auto create_zwlr_layer_shell_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::LayerShellV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::LayerShellV1> override;
    auto create_zwlr_screencopy_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::ScreencopyManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::ScreencopyManagerV1> override;
    auto create_zwlr_virtual_pointer_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::VirtualPointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::VirtualPointerManagerV1> override;
    auto create_xdg_activation_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgActivationV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgActivationV1> override;
    auto create_zxdg_decoration_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgDecorationManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgDecorationManagerV1> override;
    auto create_xdg_wm_dialog_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgWmDialogV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgWmDialogV1> override;
    auto create_zxdg_output_manager_v1(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgOutputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgOutputManagerV1> override;
    auto create_zxdg_shell_v6(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgShellV6Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgShellV6> override;
    auto create_xdg_wm_base(rust::Box<wayland::WaylandClient> client, rust::Box<wayland::XdgWmBaseMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland::XdgWmBase> override;
    auto can_view(rust::Str interface_name, rust::Box<wayland::WaylandClientId> client_id) -> bool override;

private:
    wayland::WaylandClientRegistry& registry;
    WaylandProtocolExtensionFilter const extension_filter;
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<Executor> const frame_callback_executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    wayland::WaylandServer& server;
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
    std::shared_ptr<scene::SessionLock> const session_lock;
    std::shared_ptr<input::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<input::InputDeviceHub> const input_hub;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const keyboard_observer_registrar;
    std::shared_ptr<input::Seat> const mir_seat;
    std::shared_ptr<shell::AccessibilityManager> const accessibility_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
    std::shared_ptr<DesktopFileManager> const desktop_file_manager;
    std::shared_ptr<input::InputDeviceRegistry> const input_device_registry;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<MainLoop> const main_loop;
    std::shared_ptr<shell::TokenAuthority> const token_authority;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<scene::Clipboard> const primary_selection_clipboard;
    std::shared_ptr<DragIconController> const drag_icon_controller;
    std::shared_ptr<PointerInputDispatcher> const pointer_input_dispatcher;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<input::CompositeEventFilter> const event_filter;
    OutputManager& output_manager;
    std::shared_ptr<WlSeat> const wl_seat;
    // Shared across every client's ext_foreign_toplevel_list_v1 so a surface
    // always reports the same stable identifier.
    std::shared_ptr<ForeignToplevelIdentifierMap> const foreign_toplevel_id_map;
    // Shared across every client's ext_session_lock_manager_v1 so the "only one
    // active lock" invariant is global rather than per-bind.
    std::shared_ptr<SessionLockState> const session_lock_state;
    // Shared across every client's xdg_activation_v1 so pending-token bookkeeping
    // and the keyboard/session observers are global rather than per-bind.
    std::shared_ptr<XdgActivationV1State> const xdg_activation_state;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_GLOBAL_FACTORY_H_
