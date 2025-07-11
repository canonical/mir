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

#include "mir/shell/token_authority.h"
#include "mir/graphics/platform.h"
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
#include "wp_viewporter.h"
#include "linux_drm_syncobj.h"

#include "mir/main_loop.h"
#include "mir/thread_name.h"
#include "mir/log.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/wayland.h"

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
class WlCompositor : public wayland::Compositor::Global
{
public:
    WlCompositor(
        struct wl_display* display,
        std::shared_ptr<mir::Executor> const& wayland_executor,
        std::shared_ptr<mir::Executor> const& frame_callback_executor,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : Global(display, Version<6>()),
          allocator{allocator},
          wayland_executor{wayland_executor},
          frame_callback_executor{frame_callback_executor}
    {
    }

    void on_surface_created(wl_client* client, uint32_t id, std::function<void(WlSurface*)> const& callback);

private:
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<mir::Executor> const frame_callback_executor;
    std::map<std::pair<wl_client*, uint32_t>, std::vector<std::function<void(WlSurface*)>>> surface_callbacks;

    class Instance : wayland::Compositor
    {
    public:
        Instance(wl_resource* new_resource, WlCompositor* compositor)
            : mw::Compositor{new_resource, Version<6>()},
              compositor{compositor}
        {
        }

    private:
        void create_surface(wl_resource* new_surface) override;
        void create_region(wl_resource* new_region) override;
        WlCompositor* const compositor;
    };

    void bind(wl_resource* new_resource)
    {
        new Instance{new_resource, this};
    }
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

void WlCompositor::Instance::create_surface(wl_resource* new_surface)
{
    auto const surface = new WlSurface{
        new_surface,
        compositor->wayland_executor,
        compositor->frame_callback_executor,
        compositor->allocator};
    auto const key = std::make_pair(wl_resource_get_client(new_surface), wl_resource_get_id(new_surface));
    auto const callbacks = compositor->surface_callbacks.find(key);
    if (callbacks != compositor->surface_callbacks.end())
    {
        for (auto const& callback : callbacks->second)
        {
            callback(surface);
        }
        compositor->surface_callbacks.erase(callbacks);
    }
    // Wayland ojects delete themselves
}

void WlCompositor::Instance::create_region(wl_resource* new_region)
{
    new WlRegion{new_region};
}
}
}

namespace
{
int halt_eventloop(int fd, uint32_t /*mask*/, void* data)
{
    auto display = reinterpret_cast<wl_display*>(data);
    wl_display_terminate(display);

    eventfd_t ignored;
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
    std::vector<std::shared_ptr<mg::RenderingPlatform>> const& render_platforms)
    : extension_filter{extension_filter},
      display{wl_display_create(), &cleanup_display},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
      executor{std::make_shared<WaylandExecutor>(wl_display_get_event_loop(display.get()))},
      allocator{allocator_for_display(allocator, display.get(), executor)},
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

    if (!display)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to create wl_display"});
    }

    wl_display_set_global_filter(display.get(), &wl_display_global_filter_func_thunk, this);

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
    seat_global = std::make_unique<mf::WlSeat>(
        display.get(),
        *executor,
        clock,
        input_hub,
        keyboard_observer_registrar,
        seat,
        accessibility_manager);
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
        token_authority});

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

    auto wayland_loop = wl_display_get_event_loop(display.get());

    WlClient::setup_new_client_handler(display.get(), shell, session_authorizer, [this](WlClient& client)
        {
            int const fd = wl_client_get_fd(client.raw_client());
            auto const handler_iter = connect_handlers.find(fd);

            if (handler_iter != std::end(connect_handlers))
            {
                auto const callback = handler_iter->second;
                connect_handlers.erase(handler_iter);
                callback(client.client_session());
            }
        });

    pause_source = wl_event_loop_add_fd(wayland_loop, pause_signal, WL_EVENT_READABLE, &halt_eventloop, display.get());
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
        [](wl_display* d)
        {
            mir::set_thread_name("Mir/Wayland");
            wl_display_run(d);
        },
        display.get()};
}

void mf::WaylandConnector::stop()
{
    if (eventfd_write(pause_signal, 1) < 0)
    {
        log_error("WaylandConnector::stop() failed to send IPC eventloop pause signal: %s (%i)", strerror(errno), errno);
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
                            strerror(errno),
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
                                strerror(errno),
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

bool mf::WaylandConnector::wl_display_global_filter_func_thunk(wl_client const* client, wl_global const* global, void *data)
{
    return static_cast<WaylandConnector*>(data)->wl_display_global_filter_func(client, global);
}

bool mf::WaylandConnector::wl_display_global_filter_func(wl_client const* client, wl_global const* global) const
{
    auto const* const interface = wl_global_get_interface(global);
    auto const session = get_session(const_cast<wl_client*>(client));
    return extension_filter(session, interface->name);
}

auto mir::frontend::get_session(wl_client* wl_client) -> std::shared_ptr<scene::Session>
{
    return WlClient::from(wl_client).client_session();
}

auto mf::get_session(wl_resource* surface) -> std::shared_ptr<ms::Session>
{
    // TODO: evaluate if this is actually what we want. Sometime's a surface's client's session is not the most
    // applicable session for the surface. See WlClient::client_session for details.
    return get_session(wl_resource_get_client(surface));
}
