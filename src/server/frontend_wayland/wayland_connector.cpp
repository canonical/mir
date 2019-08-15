/*
 * Copyright © 2015-2019 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wayland_connector.h"

#include "data_device.h"
#include "wayland_utils.h"
#include "wl_surface_role.h"
#include "window_wl_surface_role.h"
#include "wl_subcompositor.h"
#include "wl_surface.h"
#include "wl_seat.h"
#include "wl_region.h"

#include "null_event_sink.h"
#include "output_manager.h"
#include "wayland_executor.h"
#include "wlshmbuffer.h"

#include "wayland_wrapper.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/surface.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/frontend/wayland.h"

#include "mir/compositor/buffer_stream.h"

#include "mir/frontend/mir_client_session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface.h"
#include <mir/thread_name.h>

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/wayland_allocator.h"

#include "mir/renderer/gl/texture_target.h"
#include "mir/frontend/buffer_stream_id.h"

#include "mir/client/event.h"

#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_observer.h"

#include <system_error>
#include <sys/eventfd.h>
#include <wayland-server-core.h>
#include <unordered_map>
#include <boost/throw_exception.hpp>

#include <functional>
#include <type_traits>

#include <xkbcommon/xkbcommon.h>
#include <linux/input.h>
#include <mir/log.h>
#include <cstring>

#include "mir/fd.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <unordered_set>
#include "mir/anonymous_shm_file.h"

#if (WAYLAND_VERSION_MAJOR == 1) && (WAYLAND_VERSION_MINOR < 14)
#define MIR_NO_WAYLAND_FILTER
#endif

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mi = mir::input;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

namespace
{
struct ClientPrivate
{
    ClientPrivate(std::shared_ptr<MirClientSession> const& session, mf::Shell* shell)
        : session{session},
          shell{shell}
    {
    }

    ~ClientPrivate()
    {
        shell->close_session(session);
        /*
         * This ensures that further calls to
         * wl_client_get_destroy_listener(client, &cleanup_private)
         * - and hence session_for_client(client) - return nullptr.
         */
        wl_list_remove(&destroy_listener.link);
    }

    wl_listener destroy_listener;
    std::shared_ptr<MirClientSession> const session;
    /*
     * This shell is owned by the ClientSessionConstructor, which outlives all clients.
     */
    mf::Shell* const shell;
};

static_assert(
    std::is_standard_layout<ClientPrivate>::value,
    "ClientPrivate must be standard layout for wl_container_of to be defined behaviour");

ClientPrivate* private_from_listener(wl_listener* listener)
{
    ClientPrivate* userdata;
    return wl_container_of(listener, userdata, destroy_listener);
}

void cleanup_private(wl_listener* listener, void* /*data*/)
{
    delete private_from_listener(listener);
}

struct ClientSessionConstructor
{
    ClientSessionConstructor(std::shared_ptr<mf::Shell> const& shell,
                             std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
                             std::unordered_map<int, std::function<void(std::shared_ptr<scene::Session> const& session)>>* connect_handlers)
        : shell{shell},
          session_authorizer{session_authorizer},
          connect_handlers{connect_handlers}
    {
    }

    wl_listener construction_listener;
    wl_listener destruction_listener;
    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<mf::SessionAuthorizer> const session_authorizer;
    std::unordered_map<int, std::function<void(std::shared_ptr<scene::Session> const& session)>>* connect_handlers;

};

static_assert(
    std::is_standard_layout<ClientSessionConstructor>::value,
    "ClientSessionConstructor must be standard layout for wl_container_of to be "
    "defined behaviour.");

void create_client_session(wl_listener* listener, void* data)
{
    auto client = reinterpret_cast<wl_client*>(data);

    ClientSessionConstructor* construction_context;
    construction_context =
        wl_container_of(listener, construction_context, construction_listener);

    auto const handler_iter = construction_context->connect_handlers->find(wl_client_get_fd(client));

    std::function<void(std::shared_ptr<scene::Session> const& session)> const connection_handler =
        (handler_iter != std::end(*construction_context->connect_handlers)) ? handler_iter->second : [](auto){};

    if (handler_iter != std::end(*construction_context->connect_handlers))
        construction_context->connect_handlers->erase(handler_iter);

    pid_t client_pid;
    uid_t client_uid;
    gid_t client_gid;
    wl_client_get_credentials(client, &client_pid, &client_uid, &client_gid);

    if (!construction_context->session_authorizer->connection_is_allowed({client_pid, client_uid, client_gid}))
    {
        wl_client_destroy(client);
        return;
    }

    auto session = construction_context->shell->open_session(
        client_pid,
        "",
        std::make_shared<NullEventSink>());

    auto client_context = new ClientPrivate{session, construction_context->shell.get()};
    client_context->destroy_listener.notify = &cleanup_private;
    wl_client_add_destroy_listener(client, &client_context->destroy_listener);

    connection_handler(construction_context->shell->scene_session_for(session));
}

void cleanup_client_handler(wl_listener* listener, void*)
{
    ClientSessionConstructor* construction_context;
    construction_context = wl_container_of(listener, construction_context, destruction_listener);

    delete construction_context;
}

void setup_new_client_handler(wl_display* display, std::shared_ptr<mf::Shell> const& shell,
                              std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
                              std::unordered_map<int, std::function<void(std::shared_ptr<scene::Session> const& session)>>* connect_handlers)
{
    auto context = new ClientSessionConstructor{shell, session_authorizer, connect_handlers};
    context->construction_listener.notify = &create_client_session;

    wl_display_add_client_created_listener(display, &context->construction_listener);

    context->destruction_listener.notify = &cleanup_client_handler;
    wl_display_add_destroy_listener(display, &context->destruction_listener);
}
}

int64_t mir_input_event_get_event_time_ms(const MirInputEvent* event)
{
    return mir_input_event_get_event_time(event) / 1000000;
}

class WlCompositor : public wayland::Compositor::Global
{
public:
    WlCompositor(
        struct wl_display* display,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mg::WaylandAllocator> const& allocator)
        : Global(display, Version<4>()),
          allocator{allocator},
          executor{executor}
    {
    }

private:
    std::shared_ptr<mg::WaylandAllocator> const allocator;
    std::shared_ptr<mir::Executor> const executor;

    class Instance : wayland::Compositor
    {
    public:
        Instance(wl_resource* new_resource, WlCompositor* compositor)
            : mw::Compositor{new_resource, Version<4>()},
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

void WlCompositor::Instance::create_surface(wl_resource* new_surface)
{
    new WlSurface{new_surface, compositor->executor, compositor->allocator};
}

void WlCompositor::Instance::create_region(wl_resource* new_region)
{
    new WlRegion{new_region};
}

class WlShellSurface  : public wayland::ShellSurface, public WindowWlSurfaceRole
{
public:
    WlShellSurface(
        wl_resource* new_resource,
        WlSurface* surface,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager)
        : ShellSurface(new_resource, Version<1>()),
          WindowWlSurfaceRole{&seat, wayland::ShellSurface::client, surface, shell, output_manager}
    {
    }

    ~WlShellSurface() = default;

protected:
    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_toplevel() override
    {
    }

    void set_transient(
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto& parent_surface = *WlSurface::from(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE)
            mods.type = mir_window_type_gloss;
        mods.parent_id = parent_surface.surface_id();
        mods.aux_rect = geom::Rectangle{{x, y}, {}};
        mods.surface_placement_gravity = mir_placement_gravity_northwest;
        mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
        mods.placement_hints = mir_placement_hints_slide_x;
        mods.aux_rect_placement_offset_x = 0;
        mods.aux_rect_placement_offset_y = 0;

        apply_spec(mods);
    }

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};

    void handle_resize(std::experimental::optional<geometry::Point> const& /*new_top_left*/,
                       geometry::Size const& new_size) override
    {
        wl_shell_surface_send_configure(resource, WL_SHELL_SURFACE_RESIZE_NONE, new_size.width.as_int(),
                                        new_size.height.as_int());
    }

    void handle_close_request() override
    {
        destroy_wayland_object();
    }

    void set_fullscreen(
        uint32_t /*method*/,
        uint32_t /*framerate*/,
        std::experimental::optional<struct wl_resource*> const& output) override
    {
        WindowWlSurfaceRole::set_fullscreen(output);
    }

    void set_popup(
        struct wl_resource* /*seat*/,
        uint32_t /*serial*/,
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto& parent_surface = *WlSurface::from(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE)
            mods.type = mir_window_type_gloss;
        mods.parent_id = parent_surface.surface_id();
        mods.aux_rect = geom::Rectangle{{x, y}, {}};
        mods.surface_placement_gravity = mir_placement_gravity_northwest;
        mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
        mods.placement_hints = mir_placement_hints_slide_x;
        mods.aux_rect_placement_offset_x = 0;
        mods.aux_rect_placement_offset_y = 0;

        apply_spec(mods);
    }

    void set_maximized(std::experimental::optional<struct wl_resource*> const& output) override
    {
        (void)output;
        WindowWlSurfaceRole::set_state_now(mir_window_state_maximized);
    }

    void set_title(std::string const& title) override
    {
        WindowWlSurfaceRole::set_title(title);
    }

    void pong(uint32_t /*serial*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t /*serial*/) override
    {
        WindowWlSurfaceRole::initiate_interactive_move();
    }

    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges) override
    {
        MirResizeEdge edge = mir_resize_edge_none;

        switch (edges)
        {
        case WL_SHELL_SURFACE_RESIZE_TOP:
            edge = mir_resize_edge_north;
            break;

        case WL_SHELL_SURFACE_RESIZE_BOTTOM:
            edge = mir_resize_edge_south;
            break;

        case WL_SHELL_SURFACE_RESIZE_LEFT:
            edge = mir_resize_edge_west;
            break;

        case WL_SHELL_SURFACE_RESIZE_TOP_LEFT:
            edge = mir_resize_edge_northwest;
            break;

        case WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT:
            edge = mir_resize_edge_southwest;
            break;

        case WL_SHELL_SURFACE_RESIZE_RIGHT:
            edge = mir_resize_edge_east;
            break;

        case WL_SHELL_SURFACE_RESIZE_TOP_RIGHT:
            edge = mir_resize_edge_northeast;
            break;

        case WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT:
            edge = mir_resize_edge_southeast;
            break;

        default:;
        }

        WindowWlSurfaceRole::initiate_interactive_resize(edge);
    }

    void set_class(std::string const& /*class_*/) override
    {
    }
};

class WlShell : public wayland::Shell::Global
{
public:
    WlShell(
        wl_display* display,
        std::shared_ptr<mf::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager)
        : Global(display, Version<1>()),
          shell{shell},
          seat{seat},
          output_manager{output_manager}
    {
    }

    static auto get_window(wl_resource* window) -> std::shared_ptr<Surface>;

private:
    std::shared_ptr<mf::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

    class Instance : public wayland::Shell
    {
    public:
        Instance(wl_resource* new_resource, WlShell* shell)
            : mw::Shell{new_resource, Version<1>()},
              shell{shell}
        {
        }

    private:
        WlShell* const shell;

        void get_shell_surface(wl_resource* new_shell_surface, wl_resource* surface) override
        {
            new WlShellSurface(
                new_shell_surface,
                WlSurface::from(surface),
                shell->shell,
                shell->seat,
                shell->output_manager);
        }
    };

    void bind(wl_resource* new_resource) override
    {
        new Instance{new_resource, this};
    }
};
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

class ThrowingAllocator : public mg::WaylandAllocator
{
public:
    void bind_display(wl_display*, std::shared_ptr<mir::Executor>) override
    {
    }

    std::shared_ptr<mir::graphics::Buffer> buffer_from_resource(
        wl_resource*,
        std::function<void()>&&,
        std::function<void()>&&) override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"buffer_from_resource called on invalid allocator."}));
    }
};

std::shared_ptr<mg::WaylandAllocator> allocator_for_display(
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    wl_display* display,
    std::shared_ptr<mir::Executor> executor)
{
    if (auto allocator = std::dynamic_pointer_cast<mg::WaylandAllocator>(buffer_allocator))
    {
        try
        {
            allocator->bind_display(display, std::move(executor));
            return allocator;
        }
        catch (...)
        {
            mir::log(
                mir::logging::Severity::warning,
                "Wayland",
                std::current_exception(),
                "Failed to bind Wayland EGL display. Accelerated EGL will be unavailable.");
        }
    }
    /*
     * If we don't have a WaylandAllocator or it failed to bind to the display then there *should*
     * be no way for a client to send a non-shm buffer.
     *
     * In case a client manages to do something stupid return a valid allocator that will
     * just throw an exception (and disconnect the client) if something somehow tries to
     * import a buffer.
     */
    return std::make_shared<ThrowingAllocator>();
}
}

auto mf::create_wl_shell(wl_display* display, std::shared_ptr<Shell> const& shell, WlSeat* seat, OutputManager* const output_manager)
-> std::shared_ptr<void>
{
    return std::make_shared<mf::WlShell>(display, shell, *seat, output_manager);
}

void mf::WaylandExtensions::init(wl_display* display, std::shared_ptr<Shell> const& shell, WlSeat* seat, OutputManager* const output_manager)
{
    custom_extensions(display, shell, seat, output_manager);
}

void mf::WaylandExtensions::add_extension(std::string const name, std::shared_ptr<void> implementation)
{
    extension_protocols[std::move(name)] = std::move(implementation);
}

void mf::WaylandExtensions::custom_extensions(wl_display*, std::shared_ptr<Shell> const&, WlSeat*, OutputManager* const)
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
    optional_value<std::string> const& display_name,
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<MirDisplay> const& display_config,
    std::shared_ptr<mi::InputDeviceHub> const& input_hub,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    bool arw_socket,
    std::unique_ptr<WaylandExtensions> extensions_,
    WaylandProtocolExtensionFilter const& extension_filter)
    : display{wl_display_create(), &cleanup_display},
      pause_signal{eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)},
      executor{std::make_shared<WaylandExecutor>(wl_display_get_event_loop(display.get()))},
      allocator{allocator_for_display(allocator, display.get(), executor)},
      weak_shell{shell},
      extensions{std::move(extensions_)},
      extension_filter{extension_filter}
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

#ifndef MIR_NO_WAYLAND_FILTER
    wl_display_set_global_filter(display.get(), &wl_display_global_filter_func_thunk, this);
#else
    log_warning("Cannot set Wayland protocol filter: "
        "wl_display_set_global_filter() is unavailable in libwayland-dev "
        WAYLAND_VERSION);
#endif

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
        this->allocator);
    subcompositor_global = std::make_unique<mf::WlSubcompositor>(display.get());
    seat_global = std::make_unique<mf::WlSeat>(display.get(), input_hub, seat, executor);
    output_manager = std::make_unique<mf::OutputManager>(
        display.get(),
        display_config,
        executor);

    data_device_manager_global = mf::create_data_device_manager(display.get());

    extensions->init(display.get(), shell, seat_global.get(), output_manager.get());

    wl_display_init_shm(display.get());

    char const* wayland_display = nullptr;

    if (!display_name.is_set())
    {
        wayland_display = wl_display_add_socket_auto(display.get());
    }
    else
    {
        if (!wl_display_add_socket(display.get(), display_name.value().c_str()))
        {
            wayland_display = display_name.value().c_str();
        }
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

    auto wayland_loop = wl_display_get_event_loop(display.get());

    setup_new_client_handler(display.get(), shell, session_authorizer, &connect_handlers);

    pause_source = wl_event_loop_add_fd(wayland_loop, pause_signal, WL_EVENT_READABLE, &halt_eventloop, display.get());
}

mf::WaylandConnector::~WaylandConnector()
{
    if (dispatch_thread.joinable())
    {
        stop();
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
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to send IPC eventloop pause signal"}));
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

auto mf::WaylandConnector::socket_name() const -> optional_value<std::string>
{
    return wayland_display;
}

auto mf::WaylandConnector::get_extension(std::string const& name) const -> std::shared_ptr<void>
{
    return extensions->get_extension(name);
}

auto mf::WaylandConnector::get_wl_display() const -> wl_display*
{
    return display.get();
}

bool mf::WaylandConnector::wl_display_global_filter_func_thunk(wl_client const* client, wl_global const* global, void *data)
{
    return static_cast<WaylandConnector*>(data)->wl_display_global_filter_func(client, global);
}

bool mf::WaylandConnector::wl_display_global_filter_func(wl_client const* client, wl_global const* global) const
{
#ifndef MIR_NO_WAYLAND_FILTER
    auto const* const interface = wl_global_get_interface(global);
    auto const shell = weak_shell.lock();
    if (!shell)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid shell"));
    auto const session = get_session(const_cast<wl_client*>(client));
    return extension_filter(session, interface->name);
#else
    (void)client, (void)global;
    return true;
#endif
}

auto mir::frontend::get_mir_client_session(wl_client* client) -> std::shared_ptr<MirClientSession>
{
    auto listener = wl_client_get_destroy_listener(client, &cleanup_private);

    if (listener)
        return private_from_listener(listener)->session;

    return {};
}

auto mir::frontend::get_session(wl_client* client) -> std::shared_ptr<scene::Session>
{
    auto listener = wl_client_get_destroy_listener(client, &cleanup_private);

    if (listener)
    {
        auto client_private = private_from_listener(listener);
        return client_private->shell->scene_session_for(client_private->session);
    }

    return {};
}

auto mir::frontend::get_session(wl_resource* surface) -> std::shared_ptr<scene::Session>
{
    return get_session(wl_resource_get_client(surface));
}

auto mir::frontend::get_wl_shell_window(wl_resource* surface) -> std::shared_ptr<Surface>
{
    if (mir::wayland::Surface::is_instance(surface))
    {
        auto const wlsurface = WlSurface::from(surface);

        auto const id = wlsurface->surface_id();
        if (id.as_value())
        {
            auto const session = get_mir_client_session(wlsurface->client);
            return session->get_surface(id);
        }

        log_debug("No window currently associated with wayland::Surface %p", surface);
    }

    return {};
}
