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
#include "wayland_rs/wayland_rs_cpp/include/wayland.h"
#include "wayland_rs/wayland_rs_cpp/include/wayland_server_notification_handler.h"
#include "wl_client_registry.h"

#include <mir/shell/token_authority.h>
#include <mir/graphics/platform.h>
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
#include "desktop_file_manager.h"
#include "foreign_toplevel_manager_v1.h"
#include "wp_viewporter.h"
#include "linux_drm_syncobj.h"
#include "surface_registry.h"
#include "keyboard_state_tracker.h"

#include <mir/errno_utils.h>
#include <mir/main_loop.h>
#include <mir/thread_name.h>
#include <mir/log.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/frontend/wayland.h>

#include <future>
#include <optional>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <wayland-server-core.h>
#include <boost/throw_exception.hpp>

#include <functional>
#include <type_traits>
#include <cstring>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mi = mir::input;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
class GlobalFactory : public wayland_rs::GlobalFactory {
public:
    GlobalFactory(std::shared_ptr<WlClientRegistry> const& client_registry,
        WaylandConnector::WaylandProtocolExtensionFilter const extension_filter)
        : client_registry(client_registry),
          extension_filter(std::move(extension_filter))
    {
    }

    auto can_view(rust::String interface_name, rust::Box<wayland_rs::WaylandClientId> client_id) -> bool override
    {
        auto const client = client_registry->from_id(std::move(client_id));
        return extension_filter(client->client_session(), interface_name.c_str());
    }

    std::shared_ptr<WlClientRegistry> client_registry;
    WaylandConnector::WaylandProtocolExtensionFilter const extension_filter;
};

class WaylandServerNotificationHandler : public wayland_rs::WaylandServerNotificationHandler
{
public:
    explicit WaylandServerNotificationHandler(std::shared_ptr<WlClientRegistry> const& client_registry)
        : client_registry(client_registry)
    {}

    auto client_added(rust::Box<wayland_rs::WaylandClient> wayland_client) -> void override
    {
        client_registry->add_client(std::move(wayland_client));
    }

    auto client_removed(rust::Box<wayland_rs::WaylandClientId> id) -> void override
    {
        client_registry->delete_client(std::move(id));
    }

    std::shared_ptr<WlClientRegistry> client_registry;
};

class SurfaceRegistry
{
public:

};

class WlCompositor : public wayland_rs::WlCompositorImpl
{
public:
    WlCompositor(
        std::shared_ptr<WlClientRegistry> const& client_registry,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : client_registry{client_registry},
          allocator{allocator},
          frame_callback_executor{frame_callback_executor}
    {
    }

    void on_surface_created(wl_client* client, uint32_t id, std::function<void(WlSurface*)> const& callback);
    auto create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl> override;
    auto create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl> override;

private:
    std::shared_ptr<WlClientRegistry> const& client_registry;
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const frame_callback_executor;
    std::map<std::pair<wl_client*, uint32_t>, std::vector<std::function<void(WlSurface*)>>> surface_callbacks;
};

void WlCompositor::on_surface_created(wl_client* client, uint32_t id, std::function<void(WlSurface*)> const& callback)
{
    wl_resource* resource = wl_client_get_object(client, id);
    auto const wl_surface = resource ? WlSurface::from(resource) : nullptr;
    if (wl_surface)
    {
        callback(wl_surface);
    }
    else
    {
        surface_callbacks[std::make_pair(client, id)].push_back(callback);
    }
}

auto WlCompositor::create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl>
{
    auto const surface = std::make_shared<WlSurface>{
        client,
        frame_callback_executor,
        allocator};
    auto const key = std::make_pair(wl_resource_get_client(new_surface), wl_resource_get_id(new_surface));
    auto const callbacks = surface_callbacks.find(key);
    if (callbacks != surface_callbacks.end())
    {
        for (auto const& callback : callbacks->second)
        {
            callback(surface);
        }
        compositor->surface_callbacks.erase(callbacks);
    }
}
}
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
      client_registry{std::make_shared<WlClientRegistry>()},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
      allocator{allocator_for_display(allocator)},
      shell{shell},
      extensions{std::move(extensions_)},
      session_lock_{session_lock}
{
    if (pause_signal == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to create IPC pause notification eventfd"}));
    }

    if (!server.into_raw())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to create wl_display"});
    }

    // Run the builders before creating the seat (because that's what GTK3 expects)
    extensions->run_builders(
        display.get(),
        [executor=executor](std::function<void()>&& work) { executor->spawn(std::move(work)); });

    /*
     * Here be Dragons!
     *
     * Some clients expect a certain order in the publication of globals, and will
     * crash with a different order. Yay!
     *
     * So far I've only found ones which expect wl_compositor before anything else,
     * so stick that first.
     */
    compositor_global = std::make_unique<mf::WlCompositor>(
        display.get(),
        executor,
        std::make_shared<FrameExecutor>(*main_loop),
        this->allocator);
    subcompositor_global = std::make_unique<mf::WlSubcompositor>(display.get());
    auto const surface_registry = std::make_shared<mf::SurfaceRegistry>();

    auto const action_group_manager =
        std::make_shared<InputTriggerRegistry::ActionGroupManager>(token_authority, *executor);
    auto const input_trigger_registry = std::make_shared<frontend::InputTriggerRegistry>();
    auto const keyboard_state_tracker = std::make_shared<KeyboardStateTracker>();
    seat_global = std::make_unique<mf::WlSeat>(
        display.get(),
        *executor,
        clock,
        input_hub,
        keyboard_observer_registrar,
        seat,
        accessibility_manager,
        surface_registry,
        input_trigger_registry,
        keyboard_state_tracker);
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        executor,
        display_config_registrar);

    desktop_file_manager = std::make_shared<mf::DesktopFileManager>(
        std::make_shared<mf::GDesktopFileCache>(main_loop));

    data_device_manager_global = std::make_unique<WlDataDeviceManager>(
        display.get(),
        executor,
        main_clipboard,
        std::move(drag_icon_controller),
        std::move(pointer_input_dispatcher));

    extensions->init(WaylandExtensions::Context{
        display.get(),
        executor,
        shell,
        session_authorizer,
        main_clipboard,
        primary_selection_clipboard,
        text_input_hub,
        idle_hub,
        seat_global.get(),
        output_manager.get(),
        surface_stack,
        input_device_registry,
        composite_event_filter,
        allocator,
        screen_shooter_factory,
        main_loop,
        desktop_file_manager,
        session_lock_,
        decoration_strategy,
        session_coordinator,
        keyboard_observer_registrar,
        token_authority,
        surface_registry,
        clock,
        cursor_observer_multiplexer,
        action_group_manager,
        input_trigger_registry,
        keyboard_state_tracker});

    shm_global = std::make_unique<WlShm>(display.get(), executor);

    viewporter = std::make_unique<WpViewporter>(display.get());

    {
        std::vector<std::shared_ptr<mg::DRMRenderingProvider>> providers;
        for (auto platform : render_platforms)
        {
            if (auto provider = mg::RenderingPlatform::acquire_provider<mg::DRMRenderingProvider>(platform))
            {
                providers.push_back(std::move(provider));
            }
        }
        if (!providers.empty())
        {
            mir::log_debug("Detected DRM Syncobj capable rendering platform. Enabling linux_drm_syncobj_v1 explicit sync support.");
            drm_syncobj = std::make_unique<LinuxDRMSyncobjManager>(display.get(), providers);
        }
    }

    char const* wayland_display = nullptr;

    if (auto const display_name = getenv("WAYLAND_DISPLAY"))
    {
        if (wl_display_add_socket(display.get(), display_name) != 0)
        {
            BOOST_THROW_EXCEPTION(
                std::system_error(errno, std::system_category(),
                    "Try unsetting or changing WAYLAND_DISPLAY. The current value can not be used:"));
        }

        wayland_display = display_name;
    }
    else
    {
        wayland_display = wl_display_add_socket_auto(display.get());
    }

    if (wayland_display)
    {
        if (arw_socket)
        {
            chmod((std::string{getenv("XDG_RUNTIME_DIR")} + "/" + wayland_display).c_str(),
                  S_IRUSR|S_IWUSR| S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
        };

        this->wayland_display = wayland_display;
    }
    else
    {
        fatal_error("Unable to bind Wayland socket");
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
    wl_display_destroy_clients(display.get());

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
    dispatch_thread = std::thread{
        [
            wayland_display = wayland_display,
            client_registry = client_registry,
            extension_filter = extension_filter
        ](wayland_rs::WaylandServer* server)
        {
            mir::set_thread_name("Mir/Wayland");
            server->run(wayland_display,
                std::make_unique<GlobalFactory>(client_registry, extension_filter),
                std::make_unique<WaylandServerNotificationHandler>(client_registry));
        },
        server.into_raw()};
}

void mf::WaylandConnector::stop()
{
    server->stop();
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
    return wayland_display;
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
