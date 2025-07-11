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

#ifndef MIR_FRONTEND_WAYLAND_CONNECTOR_H_
#define MIR_FRONTEND_WAYLAND_CONNECTOR_H_

#include "mir/shell/token_authority.h"
#include "wayland_wrapper.h"

#include "mir/fd.h"
#include "mir/frontend/connector.h"
#include "mir/frontend/drag_icon_controller.h"
#include <mir/frontend/wayland.h>
#include "mir/optional_value.h"

#include <wayland-server-core.h>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <vector>

namespace mir
{
class Executor;
class MainLoop;
template<typename>
class ObserverRegistrar;
class DecorationStrategy;

namespace compositor
{
class ScreenShooterFactory;
}

namespace input
{
class InputDeviceHub;
class InputDeviceRegistry;
class Seat;
class CompositeEventFilter;
class KeyboardObserver;
}
namespace graphics
{
class GraphicBufferAllocator;
class DisplayConfigurationObserver;
class RenderingPlatform;
}
namespace shell
{
class Shell;
class AccessibilityManager;
}
namespace scene
{
class Clipboard;
class IdleHub;
class Surface;
class TextInputHub;
class SessionLock;
class SessionCoordinator;
}
namespace time
{
class Clock;
}
namespace frontend
{
class OutputManager;
class PointerInputDispatcher;
class SessionAuthorizer;
class SurfaceStack;
class WlApplication;
class WlCompositor;
class WlDataDeviceManager;
class WlSeat;
class WlShm;
class WlSubcompositor;
class WlSurface;
class WpViewporter;
class LinuxDRMSyncobjManager;
class DesktopFileManager;

class WaylandExtensions
{
public:
    /// The resources needed to init Wayland extensions
    struct Context
    {
        wl_display* display;
        std::shared_ptr<Executor> wayland_executor;
        std::shared_ptr<shell::Shell> shell;
        std::shared_ptr<SessionAuthorizer> session_authorizer;
        std::shared_ptr<scene::Clipboard> main_clipboard;
        std::shared_ptr<scene::Clipboard> primary_selection_clipboard;
        std::shared_ptr<scene::TextInputHub> text_input_hub;
        std::shared_ptr<scene::IdleHub> idle_hub;
        WlSeat* seat;
        OutputManager* output_manager;
        std::shared_ptr<SurfaceStack> surface_stack;
        std::shared_ptr<input::InputDeviceRegistry> input_device_registry;
        std::shared_ptr<input::CompositeEventFilter> composite_event_filter;
        std::shared_ptr<graphics::GraphicBufferAllocator> graphic_buffer_allocator;
        std::shared_ptr<compositor::ScreenShooterFactory> screen_shooter_factory;
        std::shared_ptr<MainLoop> main_loop;
        std::shared_ptr<DesktopFileManager> desktop_file_manager;
        std::shared_ptr<scene::SessionLock> session_lock;
        std::shared_ptr<mir::DecorationStrategy> decoration_strategy;
        std::shared_ptr<scene::SessionCoordinator> session_coordinator;
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> keyboard_observer_registrar;
        std::shared_ptr<shell::TokenAuthority> token_authority;
    };

    WaylandExtensions() = default;
    virtual ~WaylandExtensions() = default;
    WaylandExtensions(WaylandExtensions const&) = delete;
    WaylandExtensions& operator=(WaylandExtensions const&) = delete;

    virtual void run_builders(wl_display* display, std::function<void(std::function<void()>&& work)> const& run_on_wayland_mainloop);

    void init(Context const& context);

    auto get_extension(std::string const& name) const -> std::shared_ptr<void>;

protected:

    void add_extension(std::string const name, std::shared_ptr<void> implementation);
    virtual void custom_extensions(Context const& context);

private:
    std::unordered_map<std::string, std::shared_ptr<void>> extension_protocols;
};

class WaylandConnector : public Connector, public WaylandTools
{
public:
    using WaylandProtocolExtensionFilter = std::function<bool(std::shared_ptr<scene::Session> const&, char const*)>;

    WaylandConnector(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<time::Clock> const& clock,
        std::shared_ptr<input::InputDeviceHub> const& input_hub,
        std::shared_ptr<input::Seat> const& seat,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<input::CompositeEventFilter> const& composite_event_filter,
        std::shared_ptr<frontend::DragIconController> drag_icon_controller,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar,
        std::shared_ptr<scene::Clipboard> const& main_clipboard,
        std::shared_ptr<scene::Clipboard> const& primary_selection_clipboard,
        std::shared_ptr<scene::TextInputHub> const& text_input_hub,
        std::shared_ptr<scene::IdleHub> const& idle_hub,
        std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
        std::shared_ptr<MainLoop> const& main_loop,
        bool arw_socket,
        std::unique_ptr<WaylandExtensions> extensions,
        WaylandProtocolExtensionFilter const& extension_filter,
        std::shared_ptr<shell::AccessibilityManager> const& accessibility_manager,
        std::shared_ptr<scene::SessionLock> const& session_lock,
        std::shared_ptr<DecorationStrategy> const& decoration_strategy,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<shell::TokenAuthority> const& token_authority,
        std::vector<std::shared_ptr<graphics::RenderingPlatform>> const& render_platforms);

    ~WaylandConnector() override;

    void start() override;
    void stop() override;

    int client_socket_fd() const override;

    int client_socket_fd(
        std::function<void(std::shared_ptr<scene::Session> const& session)> const& connect_handler) const override;

    void run_on_wayland_display(std::function<void(wl_display*)> const& functor);

    /// Runs callback the first time a wl_surface with the given id is created, or immediately if one currently exists
    /// Callback is never called if a wl_surface with the id is never created
    void on_surface_created(wl_client* client, uint32_t id, std::function<void(WlSurface*)> const& callback);

    auto socket_name() const -> std::optional<std::string> override;

    auto get_extension(std::string const& name) const -> std::shared_ptr<void>;

private:
    void for_each_output_binding(
        wayland::Client* client,
        graphics::DisplayConfigurationOutputId output,
        std::function<void(wl_resource* output)> const& callback) override;

    bool wl_display_global_filter_func(wl_client const* client, wl_global const* global) const;
    static bool wl_display_global_filter_func_thunk(wl_client const* client, wl_global const* global, void* data);

    /* The wayland global filter is called during wl_global_remove
     * (libwayland needs to know if it should broadcast the removal)
     *
     * This means the filter needs to outlive any extension object
     * that provides a wl_global (which is all of them).
     *
     * For safety, just ensure it outlives the wl_display. libwayland
     * cannot *possibly* call it after then :)
     */
    WaylandProtocolExtensionFilter const extension_filter;

    std::unique_ptr<wl_display, void(*)(wl_display*)> const display;
    mir::Fd const pause_signal;
    std::unique_ptr<WlCompositor> compositor_global;
    std::unique_ptr<WlSubcompositor> subcompositor_global;
    std::unique_ptr<WlSeat> seat_global;
    std::unique_ptr<OutputManager> output_manager;
    std::shared_ptr<DesktopFileManager> desktop_file_manager;
    std::unique_ptr<WlDataDeviceManager> data_device_manager_global;
    std::unique_ptr<WlShm> shm_global;
    std::unique_ptr<WpViewporter> viewporter;
    std::unique_ptr<LinuxDRMSyncobjManager> drm_syncobj;
    std::shared_ptr<Executor> const executor;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;
    std::shared_ptr<shell::Shell> const shell;
    std::unique_ptr<WaylandExtensions> const extensions;
    std::shared_ptr<scene::SessionLock> session_lock_;
    std::thread dispatch_thread;
    wl_event_source* pause_source;
    std::string wayland_display;

    // Only accessed on event loop
    std::unordered_map<int, std::function<void(std::shared_ptr<scene::Session> const& session)>> mutable connect_handlers;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_CONNECTOR_H_
