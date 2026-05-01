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

#include "wayland_connector.h"
#include "wl_client_registry.h"

#include <mir/shell/token_authority.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/linux_dmabuf.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include "wl_client.h"
#include "wl_data_device_manager.h"
#include "wayland_utils.h"
#include "wl_subcompositor.h"
#include "wl_seat.h"
#include "wl_region.h"
#include "shm.h"
#include "frame_executor.h"
#include "output_manager.h"
#include "wayland_executor.h"
#include "desktop_file_manager.h"
#include "foreign_toplevel_manager_v1.h"
#include "foreign_toplevel_list_v1.h"
#include "wp_viewporter.h"
#include "linux_drm_syncobj.h"
#include "surface_registry.h"
#include "keyboard_state_tracker.h"
#include "data_control_v1.h"
#include "xdg_activation_v1.h"
#include "input_method_v1.h"
#include "input_method_v2.h"
#include "session_lock_v1.h"
#include "input_trigger_action_v1.h"
#include "input_trigger_registration_v1.h"
#include "virtual_keyboard_v1.h"
#include "virtual_pointer_v1.h"
#include "xdg_shell_stable.h"
#include "xdg_output_v1.h"
#include "xdg_decoration_unstable_v1.h"
#include "layer_shell_v1.h"
#include "wl_shell.h"
#include "relative_pointer_unstable_v1.h"
#include "pointer_constraints_unstable_v1.h"
#include "primary_selection_v1.h"
#include "text_input_v1.h"
#include "text_input_v2.h"
#include "text_input_v3.h"
#include "idle_inhibit_v1.h"
#include "fractional_scale_v1.h"
#include "ext_image_capture_v1.h"
#include "wlr_screencopy_v1.h"

#include <mir/compositor/screen_shooter.h>
#include <mir/errno_utils.h>
#include <mir/main_loop.h>
#include <mir/thread_name.h>
#include <mir/log.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/frontend/wayland.h>

#include <future>
#include <mutex>
#include <optional>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <wayland-server-core.h>
#include <boost/throw_exception.hpp>

#include <functional>
#include <type_traits>
#include <cstring>

#include "wl_compositor.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mi = mir::input;
namespace mw = mir::wayland_rs;

namespace
{
/// Shared state that bridges the WaylandEventLoopHandle from WaylandServerNotificationHandler::loop_ready()
/// to GlobalFactory::create_wl_compositor()
struct EventLoopState
{
    std::mutex mutex;
    std::optional<rust::Box<mw::WaylandEventLoopHandle>> handle;

    void set(rust::Box<mw::WaylandEventLoopHandle> h)
    {
        std::lock_guard lock{mutex};
        handle = std::move(h);
    }

    auto clone() -> rust::Box<mw::WaylandEventLoopHandle>
    {
        std::lock_guard lock{mutex};
        if (!handle)
        {
            BOOST_THROW_EXCEPTION(std::logic_error{"EventLoopHandle not yet available"});
        }
        return (*handle)->clone_box();
    }
};

class WaylandServerNotificationHandler : public mw::WaylandServerNotificationHandler
{
public:
    WaylandServerNotificationHandler(
        std::shared_ptr<mf::WlClientRegistry> const& client_registry,
        std::shared_ptr<EventLoopState> const& event_loop_state,
        std::shared_ptr<mf::WaylandExecutor> const& executor)
        : client_registry(client_registry),
          event_loop_state(event_loop_state),
          executor(executor)
    {}

    auto client_added(rust::Box<mw::WaylandClient> wayland_client) -> void override
    {
        client_registry->add_client(std::move(wayland_client));
    }

    auto client_removed(rust::Box<mw::WaylandClientId> id) -> void override
    {
        client_registry->delete_client(std::move(id));
    }

    auto loop_ready(rust::Box<mw::WaylandEventLoopHandle> event_loop_handle) -> void override
    {
        event_loop_state->set(event_loop_handle->clone_box());
        executor->set_loop_handle(std::move(event_loop_handle));
    }

    std::shared_ptr<mf::WlClientRegistry> client_registry;
    std::shared_ptr<EventLoopState> event_loop_state;
    std::shared_ptr<mf::WaylandExecutor> executor;
};

class GlobalFactory : public mw::GlobalFactory
{
public:
    GlobalFactory(
        std::shared_ptr<mf::WlClientRegistry> const& client_registry,
        std::shared_ptr<mir::Executor> const& wayland_executor,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<mir::MainLoop> const& main_loop,
        std::shared_ptr<mir::time::Clock> const& clock,
        std::shared_ptr<mir::input::InputDeviceHub> const& input_hub,
        std::shared_ptr<mir::ObserverRegistrar<mi::KeyboardObserver>> const& keyboard_observer_registrar,
        std::shared_ptr<mir::input::Seat> const& seat,
        std::shared_ptr<msh::AccessibilityManager> const& accessibility_manager,
        std::shared_ptr<mf::SurfaceRegistry> const& surface_registry,
        std::shared_ptr<mf::InputTriggerRegistry> const& input_trigger_registry,
        std::shared_ptr<mf::KeyboardStateTracker> const& keyboard_state_tracker,
        mf::WaylandConnector::WaylandProtocolExtensionFilter const& extension_filter,
        std::vector<std::shared_ptr<mg::DRMRenderingProvider>> providers,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::Clipboard> const& main_clipboard,
        std::shared_ptr<ms::Clipboard> const& primary_selection_clipboard,
        std::shared_ptr<mf::DragIconController> const& drag_icon_controller,
        std::shared_ptr<mf::PointerInputDispatcher> const& pointer_input_dispatcher,
        std::shared_ptr<mf::SurfaceStack> const& surface_stack,
        mf::OutputManager* output_manager,
        std::shared_ptr<ms::TextInputHub> const& text_input_hub,
        std::shared_ptr<ms::IdleHub> const& idle_hub,
        std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<mi::CompositeEventFilter> const& composite_event_filter,
        std::shared_ptr<mc::ScreenShooterFactory> const& screen_shooter_factory,
        std::shared_ptr<mf::DesktopFileManager> const& desktop_file_manager,
        std::shared_ptr<ms::SessionLock> const& session_lock,
        std::shared_ptr<mir::DecorationStrategy> const& decoration_strategy,
        std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<msh::TokenAuthority> const& token_authority,
        std::shared_ptr<mi::CursorObserverMultiplexer> const& cursor_observer_multiplexer,
        std::shared_ptr<mf::InputTriggerRegistry::ActionGroupManager> const& action_group_manager,
        std::shared_ptr<EventLoopState> const& event_loop_state)
        : client_registry{client_registry},
          wayland_executor{wayland_executor},
          frame_callback_executor{frame_callback_executor},
          allocator{allocator},
          extension_filter{extension_filter},
          shell{shell},
          main_clipboard{main_clipboard},
          primary_selection_clipboard{primary_selection_clipboard},
          drag_icon_controller{drag_icon_controller},
          pointer_input_dispatcher{pointer_input_dispatcher},
          surface_stack{surface_stack},
          output_manager{output_manager},
          surface_registry{surface_registry},
          text_input_hub{text_input_hub},
          idle_hub{idle_hub},
          input_device_registry{input_device_registry},
          composite_event_filter{composite_event_filter},
          screen_shooter_factory{screen_shooter_factory},
          desktop_file_manager{desktop_file_manager},
          decoration_strategy{decoration_strategy},
          cursor_observer_multiplexer{cursor_observer_multiplexer},
          clock{clock},
          event_loop_state{event_loop_state},
          compositor_global(std::make_unique<mf::WlCompositorGlobal>(
            wayland_executor,
            std::make_shared<mf::FrameExecutor>(*main_loop),
            this->allocator)),
          seat_global(std::make_unique<mf::WlSeatGlobal>(
            *wayland_executor,
            clock,
            input_hub,
            keyboard_observer_registrar,
            seat,
            accessibility_manager,
            surface_registry,
            input_trigger_registry,
            keyboard_state_tracker
          )),
          viewporter(std::make_unique<mf::WpViewporter>()),
          data_control_manager(std::make_unique<mf::DataControlManagerV1>(
            main_clipboard,
            primary_selection_clipboard)),
          foreign_toplevel_list_global(std::make_unique<mf::ExtForeignToplevelListV1Global>(
            wayland_executor,
            surface_stack,
            desktop_file_manager)),
          xdg_activation_global(mf::create_xdg_activation_v1(
            shell,
            session_coordinator,
            main_loop,
            keyboard_observer_registrar,
            *wayland_executor,
            token_authority)),
          input_method_v1(std::make_unique<mf::InputMethodV1>(
            text_input_hub,
            wayland_executor)),
          input_panel_v1(std::make_unique<mf::InputPanelV1>(
            wayland_executor,
            shell,
            seat_global.get(),
            output_manager,
            text_input_hub,
            surface_registry)),
          session_lock_manager(std::make_unique<mf::SessionLockManagerV1>(
            *wayland_executor,
            shell,
            session_lock,
            *seat_global,
            output_manager,
            surface_stack,
            surface_registry)),
          input_trigger_action_manager(mf::create_input_trigger_action_manager_v1(
            action_group_manager)),
          input_trigger_registration_manager(mf::create_input_trigger_registration_manager_v1(
            action_group_manager,
            input_trigger_registry,
            keyboard_state_tracker)),
          virtual_keyboard_ctx(std::make_shared<mf::VirtualKeyboardV1Ctx>(
            mf::VirtualKeyboardV1Ctx{input_device_registry}))
    {
        if (!providers.empty())
        {
            mir::log_debug("Detected DRM Syncobj capable rendering platform. Enabling linux_drm_syncobj_v1 explicit sync support.");
            linux_drm_sync_obj = std::make_unique<mf::LinuxDRMSyncobjManagerGlobal>(providers);
        }
    }

    auto can_view(rust::String interface_name, rust::Box<mw::WaylandClientId> client_id) -> bool override
    {
        auto const session = client_registry->from(client_id)->client_session();
        return extension_filter(session, interface_name.c_str());
    }

    // -- Direct construction globals --

    auto create_wl_compositor(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlCompositorImpl> override
    {
        return std::make_shared<mf::WlCompositor>(
            compositor_global.get(),
            client_registry->from(client),
            wayland_executor,
            frame_callback_executor,
            allocator,
            event_loop_state->clone());
    }

    auto create_wl_shm(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlShmImpl> override
    {
        (void)client;
        return std::make_shared<mf::Shm>(wayland_executor);
    }

    auto create_wl_data_device_manager(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlDataDeviceManagerImpl> override
    {
        return std::make_shared<mf::WlDataDeviceManager>(
            wayland_executor,
            main_clipboard,
            drag_icon_controller,
            pointer_input_dispatcher,
            client_registry->from(client));
    }

    auto create_wl_subcompositor(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlSubcompositorImpl> override
    {
        return std::make_shared<mf::WlSubcompositor>(client_registry->from(client));
    }

    auto create_xdg_wm_base(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::XdgWmBaseImpl> override
    {
        return std::make_shared<mf::XdgShellStable>(
            client_registry->from(client),
            *wayland_executor,
            shell,
            *seat_global,
            output_manager,
            surface_registry);
    }

    auto create_zxdg_output_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZxdgOutputManagerV1Impl> override
    {
        return std::make_shared<mf::XdgOutputManagerV1>(client_registry->from(client));
    }

    auto create_zxdg_decoration_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZxdgDecorationManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::XdgDecorationManagerV1>(
            client_registry->from(client),
            decoration_strategy);
    }

    auto create_zwlr_layer_shell_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwlrLayerShellV1Impl> override
    {
        return std::make_shared<mf::LayerShellV1>(
            client_registry->from(client),
            *wayland_executor,
            shell,
            *seat_global,
            output_manager,
            surface_registry);
    }

    auto create_wl_shell(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlShellImpl> override
    {
        return std::make_shared<mf::WlShell>(
            client_registry->from(client),
            *wayland_executor,
            shell,
            *seat_global,
            output_manager,
            surface_registry);
    }

    auto create_zwlr_foreign_toplevel_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwlrForeignToplevelManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::ForeignToplevelManagerV1>(
            shell,
            wayland_executor,
            surface_stack,
            desktop_file_manager);
    }

    auto create_zwp_relative_pointer_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpRelativePointerManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::RelativePointerManagerV1>(shell);
    }

    auto create_zwp_pointer_constraints_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpPointerConstraintsV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::PointerConstraintsV1>(*wayland_executor, shell);
    }

    auto create_zwp_primary_selection_device_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpPrimarySelectionDeviceManagerV1Impl> override
    {
        return std::make_shared<mf::PrimarySelectionManager>(
            client_registry->from(client),
            wayland_executor,
            primary_selection_clipboard);
    }

    auto create_zwp_text_input_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpTextInputManagerV1Impl> override
    {
        return std::make_shared<mf::TextInputManagerV1>(
            wayland_executor,
            text_input_hub,
            client_registry->from(client));
    }

    auto create_zwp_text_input_manager_v2(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpTextInputManagerV2Impl> override
    {
        return std::make_shared<mf::TextInputManagerV2>(
            wayland_executor,
            text_input_hub,
            client_registry->from(client));
    }

    auto create_zwp_text_input_manager_v3(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpTextInputManagerV3Impl> override
    {
        return std::make_shared<mf::TextInputManagerV3>(
            wayland_executor,
            text_input_hub,
            input_device_registry,
            client_registry->from(client));
    }

    auto create_zwp_idle_inhibit_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpIdleInhibitManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::IdleInhibitManagerV1>(wayland_executor, idle_hub);
    }

    auto create_zwp_virtual_keyboard_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpVirtualKeyboardManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::VirtualKeyboardManagerV1>(virtual_keyboard_ctx);
    }

    auto create_zwlr_virtual_pointer_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwlrVirtualPointerManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::VirtualPointerManagerV1>(output_manager, input_device_registry);
    }

    auto create_wp_fractional_scale_manager_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WpFractionalScaleManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::FractionalScaleManagerV1>();
    }

    auto create_zwp_input_method_manager_v2(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::ZwpInputMethodManagerV2Impl> override
    {
        return std::make_shared<mf::InputMethodManagerV2>(
            client_registry->from(client),
            wayland_executor,
            text_input_hub,
            composite_event_filter);
    }

    auto create_ext_output_image_capture_source_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtOutputImageCaptureSourceManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::ExtOutputImageCaptureSourceManagerV1>(
            wayland_executor,
            screen_shooter_factory,
            surface_stack);
    }

    auto create_ext_image_copy_capture_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtImageCopyCaptureManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::ExtImageCopyCaptureManagerV1>(
            wayland_executor,
            cursor_observer_multiplexer,
            clock);
    }

    auto create_zwlr_screencopy_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwlrScreencopyManagerV1Impl> override
    {
        (void)client;
        return std::make_shared<mf::WlrScreencopyManagerV1>(
            wayland_executor,
            allocator,
            screen_shooter_factory,
            surface_stack);
    }

    // -- Global class + create() pattern --

    auto create_wl_seat(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlSeatImpl> override
    {
        return seat_global->create(client_registry->from(client));
    }

    auto create_wp_viewporter(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WpViewporterImpl> override
    {
        (void)client;
        return viewporter->create();
    }

    auto create_wp_linux_drm_syncobj_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::WpLinuxDrmSyncobjManagerV1Impl> override
    {
        (void)client;
        if (linux_drm_sync_obj)
        {
            return linux_drm_sync_obj->create();
        }
        return nullptr;
    }

    auto create_ext_data_control_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtDataControlManagerV1Impl> override
    {
        (void)client;
        return data_control_manager->create();
    }

    auto create_ext_foreign_toplevel_list_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtForeignToplevelListV1Impl> override
    {
        (void)client;
        return foreign_toplevel_list_global->create();
    }

    auto create_xdg_activation_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::XdgActivationV1Impl> override
    {
        return xdg_activation_global->create(client_registry->from(client));
    }

    auto create_zwp_input_method_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwpInputMethodV1Impl> override
    {
        (void)client;
        return input_method_v1->create();
    }

    auto create_zwp_input_panel_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwpInputPanelV1Impl> override
    {
        return input_panel_v1->create(client_registry->from(client));
    }

    auto create_ext_session_lock_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtSessionLockManagerV1Impl> override
    {
        return session_lock_manager->create(client_registry->from(client));
    }

    auto create_ext_input_trigger_action_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtInputTriggerActionManagerV1Impl> override
    {
        (void)client;
        return input_trigger_action_manager->create();
    }

    auto create_ext_input_trigger_registration_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtInputTriggerRegistrationManagerV1Impl> override
    {
        return input_trigger_registration_manager->create(client_registry->from(client));
    }

    auto create_zwp_linux_dmabuf_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwpLinuxDmabufV1Impl> override
    {
        (void)client;
        if (linux_dmabuf_global)
        {
            return linux_dmabuf_global->create();
        }
        return nullptr;
    }

    // -- Not yet implemented --

    auto create_wl_output(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::WlOutputImpl> override
    {
        (void)client;
        // TODO: wl_output is managed per-display by OutputManager, needs architectural work
        BOOST_THROW_EXCEPTION(std::logic_error{"wl_output global not yet implemented in GlobalFactory"});
    }

    auto create_mir_shell_v1(rust::Box<mw::WaylandClient> client) -> std::shared_ptr<mw::MirShellV1Impl> override
    {
        (void)client;
        // TODO: MirShellV1 constructor currently takes wl_resource*, needs update
        BOOST_THROW_EXCEPTION(std::logic_error{"mir_shell_v1 global not yet implemented in GlobalFactory"});
    }

    auto create_org_kde_kwin_server_decoration_manager(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::OrgKdeKwinServerDecorationManagerImpl> override
    {
        (void)client;
        // TODO: No C++ implementation class exists for this protocol
        BOOST_THROW_EXCEPTION(std::logic_error{"org_kde_kwin_server_decoration_manager not yet implemented"});
    }

    auto create_ext_foreign_toplevel_image_capture_source_manager_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtForeignToplevelImageCaptureSourceManagerV1Impl> override
    {
        (void)client;
        // TODO: No C++ implementation class exists for this protocol
        BOOST_THROW_EXCEPTION(std::logic_error{"ext_foreign_toplevel_image_capture_source_manager_v1 not yet implemented"});
    }

    // -- Non-global interfaces (should never be bound as globals) --

    auto create_ext_data_control_offer_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtDataControlOfferV1Impl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"ext_data_control_offer_v1 is not a global"});
    }

    auto create_ext_foreign_toplevel_handle_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ExtForeignToplevelHandleV1Impl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"ext_foreign_toplevel_handle_v1 is not a global"});
    }

    auto create_zwp_input_method_context_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwpInputMethodContextV1Impl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"zwp_input_method_context_v1 is not a global"});
    }

    auto create_zwp_primary_selection_offer_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwpPrimarySelectionOfferV1Impl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"zwp_primary_selection_offer_v1 is not a global"});
    }

    auto create_wl_data_offer(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::WlDataOfferImpl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"wl_data_offer is not a global"});
    }

    auto create_zwlr_foreign_toplevel_handle_v1(rust::Box<mw::WaylandClient> client)
        -> std::shared_ptr<mw::ZwlrForeignToplevelHandleV1Impl> override
    {
        (void)client;
        BOOST_THROW_EXCEPTION(std::logic_error{"zwlr_foreign_toplevel_handle_v1 is not a global"});
    }

private:
    std::shared_ptr<mf::WlClientRegistry> client_registry;
    std::shared_ptr<mir::Executor> wayland_executor;
    std::shared_ptr<mir::Executor> frame_callback_executor;
    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    mf::WaylandConnector::WaylandProtocolExtensionFilter extension_filter;
    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<ms::Clipboard> main_clipboard;
    std::shared_ptr<ms::Clipboard> primary_selection_clipboard;
    std::shared_ptr<mf::DragIconController> drag_icon_controller;
    std::shared_ptr<mf::PointerInputDispatcher> pointer_input_dispatcher;
    std::shared_ptr<mf::SurfaceStack> surface_stack;
    mf::OutputManager* output_manager;
    std::shared_ptr<mf::SurfaceRegistry> surface_registry;
    std::shared_ptr<ms::TextInputHub> text_input_hub;
    std::shared_ptr<ms::IdleHub> idle_hub;
    std::shared_ptr<mi::InputDeviceRegistry> input_device_registry;
    std::shared_ptr<mi::CompositeEventFilter> composite_event_filter;
    std::shared_ptr<mc::ScreenShooterFactory> screen_shooter_factory;
    std::shared_ptr<mf::DesktopFileManager> desktop_file_manager;
    std::shared_ptr<mir::DecorationStrategy> decoration_strategy;
    std::shared_ptr<mi::CursorObserverMultiplexer> cursor_observer_multiplexer;
    std::shared_ptr<mir::time::Clock> clock;
    std::shared_ptr<EventLoopState> event_loop_state;

    /*
     * TODO: Publish the globals in this exact order
     * Here be Dragons!
     *
     * Some clients expect a certain order in the publication of globals, and will
     * crash with a different order. Yay!
     *
     * So far I've only found ones which expect wl_compositor before anything else,
     * so stick that first.
     */
    std::unique_ptr<mf::WlCompositorGlobal> compositor_global;
    std::unique_ptr<mf::WlSeatGlobal> seat_global;
    std::unique_ptr<mf::WpViewporter> viewporter;
    std::unique_ptr<mf::LinuxDRMSyncobjManagerGlobal> linux_drm_sync_obj;
    std::unique_ptr<mf::DataControlManagerV1> data_control_manager;
    std::unique_ptr<mf::ExtForeignToplevelListV1Global> foreign_toplevel_list_global;
    std::shared_ptr<mf::XdgActivationV1Global> xdg_activation_global;
    std::unique_ptr<mf::InputMethodV1> input_method_v1;
    std::unique_ptr<mf::InputPanelV1> input_panel_v1;
    std::unique_ptr<mf::SessionLockManagerV1> session_lock_manager;
    std::shared_ptr<mf::InputTriggerActionManagerV1Global> input_trigger_action_manager;
    std::shared_ptr<mf::InputTriggerRegistrationManagerV1Global> input_trigger_registration_manager;
    std::shared_ptr<mf::VirtualKeyboardV1Ctx> virtual_keyboard_ctx;
    std::unique_ptr<mg::LinuxDmaBufGlobal> linux_dmabuf_global;
};
}

namespace
{
int halt_eventloop(int fd, uint32_t /*mask*/, void* data)
{
    auto display = static_cast<wl_display*>(data);
    wl_display_terminate(display);

    eventfd_t ignored{};
    if (eventfd_read(fd, &ignored) < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
           errno,
           std::system_category(),
           "Failed to consume pause event notification"}));
    }
    return 0;
}
}

namespace
{
void cleanup_display(wl_display *display)
{
    wl_display_flush_clients(display);
    wl_display_destroy(display);
}

std::shared_ptr<mg::GraphicBufferAllocator> allocator_for_display(
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    wl_display* display,
    std::shared_ptr<mir::Executor> executor)
{
    try
    {
        buffer_allocator->bind_display(display, std::move(executor));
        return buffer_allocator;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failed to bind Wayland EGL display"});
    }
}
}

void mf::WaylandExtensions::init(Context const& context)
{
    custom_extensions(context);
}

void mf::WaylandExtensions::add_extension(std::string const name, std::shared_ptr<void> implementation)
{
    extension_protocols[std::move(name)] = std::move(implementation);
}

void mf::WaylandExtensions::custom_extensions(Context const&)
{
}

auto mir::frontend::WaylandExtensions::get_extension(std::string const& name) const -> std::shared_ptr<void>
{
    auto const result = extension_protocols.find(name);
    if (result != end(extension_protocols))
        return result->second;

    return {};
}

void mf::WaylandExtensions::run_builders(wl_display*, std::function<void(std::function<void()>&& work)> const&)
{
}

mf::WaylandConnector::WaylandConnector(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mi::CompositeEventFilter> const& composite_event_filter,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar,
    std::shared_ptr<ms::Clipboard> const& main_clipboard,
    std::shared_ptr<ms::Clipboard> const& primary_selection_clipboard,
    std::shared_ptr<ms::TextInputHub> const& text_input_hub,
    std::shared_ptr<ms::IdleHub> const& idle_hub,
    std::shared_ptr<mc::ScreenShooterFactory> const& screen_shooter_factory,
    std::shared_ptr<MainLoop> const& main_loop,
    bool arw_socket,
    std::unique_ptr<WaylandExtensions> extensions_,
    WaylandProtocolExtensionFilter const& extension_filter,
    std::shared_ptr<shell::AccessibilityManager> const& accessibility_manager,
    std::shared_ptr<scene::SessionLock> const& session_lock,
    std::shared_ptr<mir::DecorationStrategy> const& decoration_strategy,
    std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<shell::TokenAuthority> const& token_authority,
    std::vector<std::shared_ptr<mg::RenderingPlatform>> const& render_platforms,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer)
    : extension_filter{extension_filter},
      server{wayland_rs::create_wayland_server()},
      executor{std::make_shared<WaylandExecutor>()},
      allocator{allocator_for_display(allocator, display.get(), executor)},
      shell{shell},
      clock{clock},
      input_hub{input_hub},
      seat{seat},
      keyboard_observer_registrar{keyboard_observer_registrar},
      input_device_registry{input_device_registry},
      composite_event_filter{composite_event_filter},
      drag_icon_controller{drag_icon_controller},
      pointer_input_dispatcher{pointer_input_dispatcher},
      session_authorizer{session_authorizer},
      surface_stack{surface_stack},
      display_config_registrar{display_config_registrar},
      main_clipboard{main_clipboard},
      primary_selection_clipboard{primary_selection_clipboard},
      text_input_hub{text_input_hub},
      idle_hub{idle_hub},
      screen_shooter_factory{screen_shooter_factory},
      main_loop{main_loop},
      arw_socket{arw_socket},
      extensions{std::move(extensions_)},
      accessibility_manager{accessibility_manager},
      session_lock_{session_lock},
      decoration_strategy{decoration_strategy},
      session_coordinator{session_coordinator},
      token_authority{token_authority},
      render_platforms{render_platforms},
      cursor_observer_multiplexer{cursor_observer_multiplexer}
{
    // Run the builders before creating the seat (because that's what GTK3 expects)
    // extensions->run_builders(
    //     display.get(),
    //     [executor=executor](std::function<void()>&& work) { executor->spawn(std::move(work)); });


    surface_registry = std::make_shared<mf::SurfaceRegistry>();

    action_group_manager =
        std::make_shared<InputTriggerRegistry::ActionGroupManager>(token_authority, *executor);
    input_trigger_registry = std::make_shared<frontend::InputTriggerRegistry>();
    keyboard_state_tracker = std::make_shared<KeyboardStateTracker>();
    output_manager = std::make_unique<mf::OutputManager>(
        executor,
        display_config_registrar);

    desktop_file_manager = std::make_shared<mf::DesktopFileManager>(
        std::make_shared<mf::GDesktopFileCache>(main_loop));

    for (auto const& platform : render_platforms)
    {
        if (auto provider = mg::RenderingPlatform::acquire_provider<mg::DRMRenderingProvider>(platform))
        {
            drm_rendering_providers.push_back(std::move(provider));
        }
    }
}

namespace
{
void unbind_display(mg::GraphicBufferAllocator& allocator, wl_display* display) noexcept
try
{
    allocator.unbind_display(display);
}
catch (...)
{
    mir::log(
        mir::logging::Severity::warning,
        MIR_LOG_COMPONENT,
        std::current_exception(),
        "Failed to unbind EGL display");
}
}

mf::WaylandConnector::~WaylandConnector()
{
    if (dispatch_thread.joinable())
    {
        auto cleanup_promise = std::make_shared<std::promise<void>>();
        auto cleanup_done = cleanup_promise->get_future();
        executor->spawn(
            [cleanup_promise, this]()
            {
                unbind_display(*allocator, display.get());
                cleanup_promise->set_value();
            });
        cleanup_done.get();

        stop();
    }
    else
    {
        unbind_display(*allocator, display.get());
    }
    wl_event_source_remove(pause_source);
}

void mf::WaylandConnector::start()
{
    auto event_loop_state = std::make_shared<EventLoopState>();

    dispatch_thread = std::thread{
        [&](wayland_rs::WaylandServer* server)
        {
            mir::set_thread_name("Mir/Wayland");

            // TODO: Resolve the socket
            server->run("wayland-98",
                std::make_unique<GlobalFactory>(
                    client_registry,
                    executor,
                    nullptr, // TODO
                    allocator,
                    main_loop,
                    clock,
                    input_hub,
                    keyboard_observer_registrar,
                    seat,
                    accessibility_manager,
                    surface_registry,
                    input_trigger_registry,
                    keyboard_state_tracker,
                    extension_filter,
                    drm_rendering_providers,
                    shell,
                    main_clipboard,
                    primary_selection_clipboard,
                    drag_icon_controller,
                    pointer_input_dispatcher,
                    surface_stack,
                    output_manager.get(),
                    text_input_hub,
                    idle_hub,
                    input_device_registry,
                    composite_event_filter,
                    screen_shooter_factory,
                    desktop_file_manager,
                    session_lock_,
                    decoration_strategy,
                    session_coordinator,
                    token_authority,
                    cursor_observer_multiplexer,
                    action_group_manager,
                    event_loop_state
                ),
                std::make_unique<WaylandServerNotificationHandler>(
                    client_registry,
                    event_loop_state,
                    executor
                ));
        },
        server.into_raw()};
}

void mf::WaylandConnector::stop()
{
    if (eventfd_write(pause_signal, 1) < 0)
    {
        log_error("WaylandConnector::stop() failed to send IPC eventloop pause signal: %s (%i)", mir::errno_to_cstr(errno), errno);
    }
    if (dispatch_thread.joinable())
    {
        dispatch_thread.join();
        dispatch_thread = std::thread{};
    }
    else
    {
        mir::log_warning("WaylandConnector::stop() called on not-running connector?");
    }
}

int mf::WaylandConnector::client_socket_fd() const
{
    enum { server, client, size };
    int socket_fd[size];

    char const* error = nullptr;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fd))
    {
        error = "Could not create socket pair";
    }
    else
    {
        // TODO: Wait on the result of wl_client_create so we can throw an exception on failure.
        executor->spawn(
                [socket = socket_fd[server], display = display.get()]()
                {
                    if (!wl_client_create(display, socket))
                    {
                        mir::log_error(
                            "Failed to create Wayland client object: %s (errno %i)",
                            mir::errno_to_cstr(errno),
                            errno);
                    }
                });
    }

    if (error)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), error}));

    return socket_fd[client];
}

int mf::WaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<scene::Session> const& session)> const& connect_handler) const
{
    enum { server, client, size };
    int socket_fd[size];

    char const* error = nullptr;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fd))
    {
        error = "Could not create socket pair";
    }
    else
    {
        executor->spawn(
                [socket = socket_fd[server], display = display.get(), this, connect_handler]()
                    {
                        connect_handlers[socket] = std::move(connect_handler);
                        if (!wl_client_create(display, socket))
                        {
                            mir::log_error(
                                "Failed to create Wayland client object: %s (errno %i)",
                                mir::errno_to_cstr(errno),
                                errno);
                        }
                    });
        // TODO: Wait on the result of wl_client_create so we can throw an exception on failure.
    }

    if (error)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), error}));

    return socket_fd[client];
}

void mf::WaylandConnector::run_on_wayland_display(std::function<void(wl_display*)> const& functor)
{
    executor->spawn([display_ref = display.get(), functor]() { functor(display_ref); });
}

void mf::WaylandConnector::on_surface_created(
    wl_client* client,
    uint32_t id,
    std::function<void(WlSurface*)> const& callback)
{
    compositor_global->on_surface_created(client, id, callback);
}

auto mf::WaylandConnector::socket_name() const -> std::optional<std::string>
{
    return "wayland-98";
}

auto mf::WaylandConnector::get_extension(std::string const& name) const -> std::shared_ptr<void>
{
    return extensions->get_extension(name);
}

void mf::WaylandConnector::for_each_output_binding(
    wayland::Client* client,
    graphics::DisplayConfigurationOutputId output,
    std::function<void(wl_resource* output)> const& callback)
{
    if (auto const& og = output_manager->output_for(output))
    {
        (*og)->for_each_output_bound_by(client, [&](OutputInstance* o) { callback(o->resource); });
    }
}

auto mir::frontend::get_session(wl_client const* wl_client) -> std::shared_ptr<scene::Session>
{
    return WlClient::from(wl_client).client_session();
}

auto mf::get_session(wl_resource* surface) -> std::shared_ptr<ms::Session>
{
    // TODO: evaluate if this is actually what we want. Sometime's a surface's client's session is not the most
    // applicable session for the surface. See WlClient::client_session for details.
    return get_session(wl_resource_get_client(surface));
}
