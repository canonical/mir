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

#include "wayland_global_factory.h"

#include "shm.h"
#include "wl_compositor.h"
#include "wl_seat.h"
#include "wl_subcompositor.h"
#include "wp_viewporter.h"
#include "wp_fractional_scale_v1.h"
#include "linux_drm_syncobj.h"
#include "zwp_linux_dmabuf_v1.h"
#include "idle_inhibit_v1.h"
#include "server_decoration_manager.h"
#include "zwp_pointer_constraints_v1.h"
#include "zwp_relative_pointer_v1.h"
#include "virtual_keyboard_v1.h"
#include "virtual_pointer_v1.h"
#include "output_manager.h"
#include "input_trigger_action_v1.h"
#include "input_trigger_registration_v1.h"
#include "xdg_output_v1.h"
#include "wlr_screencopy_v1.h"
#include "ext_image_capture_v1.h"
#include "ext_output_image_capture_source_v1.h"
#include "ext_foreign_toplevel_image_capture_source_v1.h"
#include "foreign_toplevel_list_v1.h"
#include "foreign_toplevel_manager_v1.h"
#include "xdg_shell_stable.h"
#include "wl_shell.h"
#include "xdg_decoration_manager_v1.h"
#include "mir_shell.h"
#include "session_lock_v1.h"
#include "layer_shell_v1.h"
#include "xdg_activation_unstable_v1.h"
#include "wl_data_device_manager.h"
#include "primary_selection_v1.h"
#include "data_control_v1.h"
#include "text_input_v1.h"
#include "text_input_v2.h"
#include "text_input_v3.h"
#include "input_method_v1.h"
#include "input_method_v2.h"

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_client_registry.h"

#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/scene/session.h>

#include <stdexcept>
#include <string>

namespace mf = mir::frontend;
namespace mwrs = mir::wayland_rs;
namespace mg = mir::graphics;

mf::WaylandGlobalFactory::WaylandGlobalFactory(
    mwrs::WaylandClientRegistry& registry,
    WaylandProtocolExtensionFilter extension_filter,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<Executor> frame_callback_executor,
    std::shared_ptr<mg::GraphicBufferAllocator> allocator,
    mwrs::WaylandServer& server,
    std::shared_ptr<std::vector<std::shared_ptr<mg::DRMRenderingProvider>> const> drm_providers,
    std::shared_ptr<scene::IdleHub> idle_hub,
    std::shared_ptr<DecorationStrategy> decoration_strategy,
    std::shared_ptr<shell::Shell> shell,
    std::shared_ptr<InputTriggerRegistry::ActionGroupManager> action_group_manager,
    std::shared_ptr<InputTriggerRegistry> input_trigger_registry,
    std::shared_ptr<KeyboardStateTracker> keyboard_state_tracker,
    std::shared_ptr<std::vector<mg::DRMFormat> const> shm_formats,
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
    OutputManager& output_manager)
    : registry{registry},
      extension_filter{std::move(extension_filter)},
      wayland_executor{std::move(wayland_executor)},
      frame_callback_executor{std::move(frame_callback_executor)},
      allocator{std::move(allocator)},
      server{server},
      drm_providers{std::move(drm_providers)},
      idle_hub{std::move(idle_hub)},
      decoration_strategy{std::move(decoration_strategy)},
      shell{std::move(shell)},
      action_group_manager{std::move(action_group_manager)},
      input_trigger_registry{std::move(input_trigger_registry)},
      keyboard_state_tracker{std::move(keyboard_state_tracker)},
      shm_formats{std::move(shm_formats)},
      screen_shooter_factory{std::move(screen_shooter_factory)},
      surface_stack{std::move(surface_stack)},
      session_lock{std::move(session_lock)},
      cursor_observer_multiplexer{std::move(cursor_observer_multiplexer)},
      clock{std::move(clock)},
      input_hub{std::move(input_hub)},
      keyboard_observer_registrar{std::move(keyboard_observer_registrar)},
      mir_seat{std::move(mir_seat)},
      accessibility_manager{std::move(accessibility_manager)},
      surface_registry{std::move(surface_registry)},
      desktop_file_manager{std::move(desktop_file_manager)},
      input_device_registry{std::move(input_device_registry)},
      session_coordinator{std::move(session_coordinator)},
      main_loop{std::move(main_loop)},
      token_authority{std::move(token_authority)},
      clipboard{std::move(clipboard)},
      primary_selection_clipboard{std::move(primary_selection_clipboard)},
      drag_icon_controller{std::move(drag_icon_controller)},
      pointer_input_dispatcher{std::move(pointer_input_dispatcher)},
      text_input_hub{std::move(text_input_hub)},
      event_filter{std::move(event_filter)},
      output_manager{output_manager},
      wl_seat{std::make_shared<WlSeat>(
          *this->wayland_executor,
          this->clock,
          this->input_hub,
          this->keyboard_observer_registrar,
          this->mir_seat,
          this->accessibility_manager,
          this->surface_registry,
          this->input_trigger_registry,
          this->keyboard_state_tracker)},
      foreign_toplevel_id_map{std::make_shared<ForeignToplevelIdentifierMap>()},
      session_lock_state{std::make_shared<SessionLockState>(this->session_lock)},
      xdg_activation_state{std::make_shared<XdgActivationV1State>(
          this->shell,
          this->session_coordinator,
          this->main_loop,
          this->keyboard_observer_registrar,
          *this->wayland_executor,
          this->token_authority)}
{
}

auto mf::WaylandGlobalFactory::can_view(rust::Str interface_name, rust::Box<mwrs::WaylandClientId> client_id) -> bool
{
    auto const client = registry.from(client_id);
    if (!client)
    {
        // We have no registered client (e.g. it was not authorized), so it may
        // not see any global.
        return false;
    }

    if (!extension_filter)
    {
        return true;
    }

    std::string const name{interface_name};
    return extension_filter(client->client_session(), name.c_str());
}

auto mf::WaylandGlobalFactory::create_ext_data_control_manager_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::ExtDataControlManagerV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtDataControlManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_data_control_manager_v1 called for an unregistered client"};

    return mf::create_ext_data_control_manager_v1(
        resolved, std::move(instance), object_id,
        clipboard, primary_selection_clipboard);
}

auto mf::WaylandGlobalFactory::create_ext_data_control_offer_v1(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::ExtDataControlOfferV1Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::ExtDataControlOfferV1>
{
    throw std::logic_error{"WaylandGlobalFactory::create_ext_data_control_offer_v1 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_ext_foreign_toplevel_list_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtForeignToplevelListV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtForeignToplevelListV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_foreign_toplevel_list_v1 called for an unregistered client"};

    return mf::create_ext_foreign_toplevel_list_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, surface_stack, desktop_file_manager, foreign_toplevel_id_map);
}

auto mf::WaylandGlobalFactory::create_ext_foreign_toplevel_handle_v1(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::ExtForeignToplevelHandleV1Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::ExtForeignToplevelHandleV1>
{
    throw std::logic_error{"WaylandGlobalFactory::create_ext_foreign_toplevel_handle_v1 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_ext_output_image_capture_source_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtOutputImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtOutputImageCaptureSourceManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_output_image_capture_source_manager_v1 called for an unregistered client"};

    return mf::create_ext_output_image_capture_source_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, screen_shooter_factory, surface_stack);
}

auto mf::WaylandGlobalFactory::create_ext_foreign_toplevel_image_capture_source_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtForeignToplevelImageCaptureSourceManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtForeignToplevelImageCaptureSourceManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_foreign_toplevel_image_capture_source_manager_v1 called for an unregistered client"};

    return mf::create_ext_foreign_toplevel_image_capture_source_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, screen_shooter_factory, surface_stack);
}

auto mf::WaylandGlobalFactory::create_ext_image_copy_capture_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtImageCopyCaptureManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtImageCopyCaptureManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_image_copy_capture_manager_v1 called for an unregistered client"};

    return mf::create_ext_image_copy_capture_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, cursor_observer_multiplexer, clock);
}

auto mf::WaylandGlobalFactory::create_ext_input_trigger_action_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtInputTriggerActionManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtInputTriggerActionManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_input_trigger_action_manager_v1 called for an unregistered client"};

    return std::make_shared<InputTriggerActionManagerV1>(resolved, std::move(instance), object_id, action_group_manager);
}

auto mf::WaylandGlobalFactory::create_ext_input_trigger_registration_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ExtInputTriggerRegistrationManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtInputTriggerRegistrationManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_input_trigger_registration_manager_v1 called for an unregistered client"};

    return std::make_shared<InputTriggerRegistrationManagerV1>(
        resolved, std::move(instance), object_id, action_group_manager, input_trigger_registry, keyboard_state_tracker);
}

auto mf::WaylandGlobalFactory::create_ext_session_lock_manager_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::ExtSessionLockManagerV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::ExtSessionLockManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_ext_session_lock_manager_v1 called for an unregistered client"};

    return mf::create_session_lock_manager_v1(
        resolved,
        std::move(instance),
        object_id,
        *wayland_executor,
        shell,
        session_lock,
        *wl_seat,
        &output_manager,
        surface_registry,
        session_lock_state);
}

auto mf::WaylandGlobalFactory::create_wp_fractional_scale_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::FractionalScaleManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::FractionalScaleManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wp_fractional_scale_manager_v1 called for an unregistered client"};

    return std::make_shared<FractionalScaleManagerV1>(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_zwp_idle_inhibit_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::IdleInhibitManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::IdleInhibitManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_idle_inhibit_manager_v1 called for an unregistered client"};

    auto ctx = std::make_shared<IdleInhibitV1Ctx>(IdleInhibitV1Ctx{wayland_executor, idle_hub});
    return std::make_shared<IdleInhibitManagerV1>(resolved, std::move(instance), object_id, std::move(ctx));
}

auto mf::WaylandGlobalFactory::create_zwp_input_method_context_v1(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::InputMethodContextV1Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::InputMethodContextV1>
{
    throw std::logic_error{"WaylandGlobalFactory::create_zwp_input_method_context_v1 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_zwp_input_method_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputMethodV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputMethodV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_input_method_v1 called for an unregistered client"};

    return mf::create_zwp_input_method_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, text_input_hub);
}

auto mf::WaylandGlobalFactory::create_zwp_input_panel_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputPanelV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputPanelV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_input_panel_v1 called for an unregistered client"};

    return mf::create_zwp_input_panel_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, shell, wl_seat.get(), &output_manager, text_input_hub, surface_registry);
}

auto mf::WaylandGlobalFactory::create_zwp_input_method_manager_v2(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::InputMethodManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::InputMethodManagerV2>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_input_method_manager_v2 called for an unregistered client"};

    return mf::create_zwp_input_method_manager_v2(
        resolved, std::move(instance), object_id,
        wayland_executor, text_input_hub, event_filter);
}

auto mf::WaylandGlobalFactory::create_zwp_linux_dmabuf_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::LinuxDmabufV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::LinuxDmabufV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_linux_dmabuf_v1 called for an unregistered client"};

    return std::make_shared<mf::LinuxDmabufV1>(
        resolved, std::move(instance), object_id, allocator->dma_buf_provider());
}

auto mf::WaylandGlobalFactory::create_wp_linux_drm_syncobj_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::LinuxDrmSyncobjManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::LinuxDrmSyncobjManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wp_linux_drm_syncobj_manager_v1 called for an unregistered client"};

    return std::make_shared<LinuxDRMSyncobjManager>(resolved, std::move(instance), object_id, drm_providers);
}

auto mf::WaylandGlobalFactory::create_mir_shell_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::MirShellV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::MirShellV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_mir_shell_v1 called for an unregistered client"};

    return mf::create_mir_shell_v1(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_zwp_pointer_constraints_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::PointerConstraintsV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::PointerConstraintsV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_pointer_constraints_v1 called for an unregistered client"};

    return std::make_shared<PointerConstraintsV1>(resolved, std::move(instance), object_id, wayland_executor, shell);
}

auto mf::WaylandGlobalFactory::create_zwp_primary_selection_device_manager_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::PrimarySelectionDeviceManagerV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::PrimarySelectionDeviceManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_primary_selection_device_manager_v1 called for an unregistered client"};

    return mf::create_zwp_primary_selection_device_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, primary_selection_clipboard);
}

auto mf::WaylandGlobalFactory::create_zwp_primary_selection_offer_v1(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::PrimarySelectionOfferV1Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::PrimarySelectionOfferV1>
{
    throw std::logic_error{"WaylandGlobalFactory::create_zwp_primary_selection_offer_v1 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_zwp_relative_pointer_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::RelativePointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::RelativePointerManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_relative_pointer_manager_v1 called for an unregistered client"};

    return mf::create_relative_pointer_unstable_v1(resolved, std::move(instance), object_id, shell);
}

auto mf::WaylandGlobalFactory::create_org_kde_kwin_server_decoration_manager(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ServerDecorationManagerMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ServerDecorationManager>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_org_kde_kwin_server_decoration_manager called for an unregistered client"};

    return std::make_shared<ServerDecorationManager>(resolved, std::move(instance), object_id, decoration_strategy);
}

auto mf::WaylandGlobalFactory::create_zwp_text_input_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_text_input_manager_v1 called for an unregistered client"};

    return mf::create_zwp_text_input_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, text_input_hub);
}

auto mf::WaylandGlobalFactory::create_zwp_text_input_manager_v2(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV2Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV2>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_text_input_manager_v2 called for an unregistered client"};

    return mf::create_zwp_text_input_manager_v2(
        resolved, std::move(instance), object_id,
        wayland_executor, text_input_hub);
}

auto mf::WaylandGlobalFactory::create_zwp_text_input_manager_v3(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::TextInputManagerV3Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::TextInputManagerV3>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_text_input_manager_v3 called for an unregistered client"};

    return mf::create_zwp_text_input_manager_v3(
        resolved, std::move(instance), object_id,
        wayland_executor, text_input_hub, input_device_registry);
}

auto mf::WaylandGlobalFactory::create_wp_viewporter(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ViewporterMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Viewporter>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wp_viewporter called for an unregistered client"};

    return std::make_shared<WpViewporter>(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_zwp_virtual_keyboard_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::VirtualKeyboardManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::VirtualKeyboardManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwp_virtual_keyboard_manager_v1 called for an unregistered client"};

    return mf::create_virtual_keyboard_manager_v1(resolved, std::move(instance), object_id, input_device_registry);
}

auto mf::WaylandGlobalFactory::create_wl_compositor(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::CompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Compositor>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wl_compositor called for an unregistered client"};

    return std::make_shared<WlCompositor>(
        resolved,
        std::move(instance),
        object_id,
        wayland_executor,
        frame_callback_executor,
        allocator,
        server);
}

auto mf::WaylandGlobalFactory::create_wl_shm(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ShmMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Shm>
{
    auto const resolved = registry.from(client);
    if (!resolved)
    {
        throw std::logic_error{"create_wl_shm called for an unregistered client"};
    }

    return std::make_shared<Shm>(
        resolved,
        std::move(instance),
        object_id,
        wayland_executor,
        shm_formats);
}

auto mf::WaylandGlobalFactory::create_wl_data_offer(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::DataOfferMiddleware>, uint32_t) -> std::shared_ptr<wayland_rs::DataOffer>
{
    throw std::logic_error{"WaylandGlobalFactory::create_wl_data_offer is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_wl_data_device_manager(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::DataDeviceManagerMiddleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::DataDeviceManager>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wl_data_device_manager called for an unregistered client"};

    return mf::create_wl_data_device_manager(
        resolved, std::move(instance), object_id,
        wayland_executor, clipboard, drag_icon_controller, pointer_input_dispatcher);
}

auto mf::WaylandGlobalFactory::create_wl_shell(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::ShellMiddleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::Shell>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wl_shell called for an unregistered client"};

    return mf::create_wl_shell(
        resolved,
        std::move(instance),
        object_id,
        *wayland_executor,
        shell,
        wl_seat.get(),
        &output_manager,
        surface_registry);
}

auto mf::WaylandGlobalFactory::create_wl_seat(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::SeatMiddleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::Seat>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wl_seat called for an unregistered client"};

    return wl_seat->create_instance(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_wl_output(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::OutputMiddleware>, uint32_t) -> std::shared_ptr<wayland_rs::Output>
{
    throw std::logic_error{"WaylandGlobalFactory::create_wl_output is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_wl_subcompositor(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::SubcompositorMiddleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::Subcompositor>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_wl_subcompositor called for an unregistered client"};

    return std::make_shared<WlSubcompositor>(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_zwlr_foreign_toplevel_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ForeignToplevelManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ForeignToplevelManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwlr_foreign_toplevel_manager_v1 called for an unregistered client"};

    return mf::create_foreign_toplevel_manager_v1(
        resolved, std::move(instance), object_id,
        shell, wayland_executor, surface_stack, desktop_file_manager);
}

auto mf::WaylandGlobalFactory::create_zwlr_foreign_toplevel_handle_v1(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::ForeignToplevelHandleV1Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::ForeignToplevelHandleV1>
{
    throw std::logic_error{"WaylandGlobalFactory::create_zwlr_foreign_toplevel_handle_v1 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_zwlr_layer_shell_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::LayerShellV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::LayerShellV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwlr_layer_shell_v1 called for an unregistered client"};

    return mf::create_layer_shell_v1(
        resolved, std::move(instance), object_id,
        *wayland_executor, shell, *wl_seat, &output_manager, surface_registry);
}

auto mf::WaylandGlobalFactory::create_zwlr_screencopy_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::ScreencopyManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::ScreencopyManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwlr_screencopy_manager_v1 called for an unregistered client"};

    return create_wlr_screencopy_manager_v1(
        resolved, std::move(instance), object_id,
        wayland_executor, allocator, screen_shooter_factory, surface_stack);
}

auto mf::WaylandGlobalFactory::create_zwlr_virtual_pointer_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::VirtualPointerManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::VirtualPointerManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zwlr_virtual_pointer_manager_v1 called for an unregistered client"};

    return mf::create_virtual_pointer_manager_v1(resolved, std::move(instance), object_id, &output_manager, input_device_registry);
}

auto mf::WaylandGlobalFactory::create_xdg_activation_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::XdgActivationV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgActivationV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_xdg_activation_v1 called for an unregistered client"};

    return mf::create_xdg_activation_v1(resolved, std::move(instance), object_id, *xdg_activation_state);
}

auto mf::WaylandGlobalFactory::create_zxdg_decoration_manager_v1(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::XdgDecorationManagerV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgDecorationManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zxdg_decoration_manager_v1 called for an unregistered client"};

    return mf::create_xdg_decoration_manager_v1(
        resolved, std::move(instance), object_id, decoration_strategy);
}

auto mf::WaylandGlobalFactory::create_zxdg_output_manager_v1(rust::Box<wayland_rs::WaylandClient> client, rust::Box<wayland_rs::XdgOutputManagerV1Middleware> instance, uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgOutputManagerV1>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_zxdg_output_manager_v1 called for an unregistered client"};

    return std::make_shared<XdgOutputManagerV1>(resolved, std::move(instance), object_id);
}

auto mf::WaylandGlobalFactory::create_zxdg_shell_v6(rust::Box<wayland_rs::WaylandClient>, rust::Box<wayland_rs::XdgShellV6Middleware>, uint32_t) -> std::shared_ptr<wayland_rs::XdgShellV6>
{
    throw std::logic_error{"WaylandGlobalFactory::create_zxdg_shell_v6 is not yet implemented"};
}

auto mf::WaylandGlobalFactory::create_xdg_wm_base(
    rust::Box<wayland_rs::WaylandClient> client,
    rust::Box<wayland_rs::XdgWmBaseMiddleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::XdgWmBase>
{
    auto const resolved = registry.from(client);
    if (!resolved)
        throw std::logic_error{"create_xdg_wm_base called for an unregistered client"};

    return mf::create_xdg_shell_stable(
        resolved,
        std::move(instance),
        object_id,
        *wayland_executor,
        shell,
        *wl_seat,
        &output_manager,
        surface_registry);
}
