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

#include "wayland_executor.h"
#include "wayland_rs/src/ffi.rs.h"
#include "wayland_rs/wayland_rs_cpp/include/wayland_executor.h"
#include "wayland_rs/wayland_rs_cpp/include/wayland_client_registry.h"

#include "wayland_global_factory.h"
#include "wayland_client.h"
#include "wayland_client_notifier.h"
#include "work_callback.h"
#include "output_manager.h"
#include "desktop_file_manager.h"
#include "surface_registry.h"
#include "keyboard_state_tracker.h"
#include "frame_executor.h"
#include "foreign_toplevel_manager_v1.h"

#include <memory>
#include <mir/fatal.h>
#include <mir/log.h>
#include <mir/main_loop.h>
#include <mir/thread_name.h>
#include <mir/shell/token_authority.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/graphics/drm_formats.h>
#include <mir/frontend/wayland.h>

#include <boost/throw_exception.hpp>

#include <atomic>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mwrs = mir::wayland_rs;

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

struct mf::WaylandConnector::ServerWrapper
{
    rust::Box<mwrs::WaylandServer> server;
};

namespace
{
/// Choose the Wayland socket name to bind to, mimicking
/// `wl_display_add_socket_auto`: honour `$WAYLAND_DISPLAY` if set, otherwise
/// pick the first `wayland-N` whose socket file does not already exist.
auto choose_wayland_socket_name() -> std::string
{
    if (auto const display_name = std::getenv("WAYLAND_DISPLAY"))
        return display_name;

    auto const runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir)
        return "wayland-0";

    for (int i = 0; i <= 32; ++i)
    {
        auto candidate = "wayland-" + std::to_string(i);
        auto const path = std::string{runtime_dir} + "/" + candidate;
        struct stat st{};
        if (::stat(path.c_str(), &st) != 0)
            return candidate;
    }

    return "wayland-0";
}

class WaylandWorkCallback : public mir::wayland_rs::WorkCallback
{
public:
    explicit WaylandWorkCallback(std::shared_ptr<mir::wayland_rs::WaylandExecutor> const& executor)
        : executor(executor)
    {
    }

    void execute() override
    {
        if (auto const locked = executor.lock())
            locked->execute();
    }

private:
    std::weak_ptr<mir::wayland_rs::WaylandExecutor> executor;
};
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
      server_wrapper{std::make_unique<ServerWrapper>(mwrs::create_wayland_server())},
      allocator{allocator},
      shell{shell},
      extensions{std::move(extensions_)},
      session_lock_{session_lock}
{
    auto& server = *server_wrapper->server;

    registry = std::make_unique<mwrs::WaylandClientRegistry>();

    // The executor doubles as the server's WorkCallback. Ownership is handed to
    // WaylandServer::run() in start(); until then we keep a non-owning view so
    // the factory and output manager can queue work onto the event loop.
    executor = std::make_shared<mwrs::WaylandExecutor>(server);

    output_manager = std::make_unique<OutputManager>(
        server,
        *registry,
        executor,
        display_config_registrar);

    desktop_file_manager = std::make_shared<mf::DesktopFileManager>(
        std::make_shared<mf::GDesktopFileCache>(main_loop));

    auto const surface_registry = std::make_shared<mf::SurfaceRegistry>();
    auto const action_group_manager =
        std::make_shared<InputTriggerRegistry::ActionGroupManager>(token_authority, *executor);
    auto const input_trigger_registry = std::make_shared<frontend::InputTriggerRegistry>();
    auto const keyboard_state_tracker = std::make_shared<KeyboardStateTracker>();
    auto const frame_callback_executor = std::make_shared<FrameExecutor>(*main_loop);

    auto shm_formats_vec = std::vector<mg::DRMFormat>{};
    for (auto const pixel_format : allocator->supported_pixel_formats())
        shm_formats_vec.push_back(mg::DRMFormat::from_mir_format(pixel_format));
    auto const shm_formats =
        std::make_shared<std::vector<mg::DRMFormat> const>(std::move(shm_formats_vec));

    auto drm_providers_vec = std::vector<std::shared_ptr<mg::DRMRenderingProvider>>{};
    for (auto const& platform : render_platforms)
    {
        if (auto provider = mg::RenderingPlatform::acquire_provider<mg::DRMRenderingProvider>(platform))
            drm_providers_vec.push_back(std::move(provider));
    }
    auto const drm_providers =
        std::make_shared<std::vector<std::shared_ptr<mg::DRMRenderingProvider>> const>(
            std::move(drm_providers_vec));

    pending_factory = std::make_unique<WaylandGlobalFactory>(
        *registry,
        extension_filter,
        executor,
        frame_callback_executor,
        allocator,
        server,
        drm_providers,
        idle_hub,
        decoration_strategy,
        shell,
        action_group_manager,
        input_trigger_registry,
        keyboard_state_tracker,
        shm_formats,
        screen_shooter_factory,
        surface_stack,
        session_lock,
        cursor_observer_multiplexer,
        clock,
        input_hub,
        keyboard_observer_registrar,
        seat,
        accessibility_manager,
        surface_registry,
        desktop_file_manager,
        input_device_registry,
        session_coordinator,
        main_loop,
        token_authority,
        main_clipboard,
        primary_selection_clipboard,
        drag_icon_controller,
        pointer_input_dispatcher,
        text_input_hub,
        composite_event_filter,
        *output_manager);

    auto const serial_source = std::make_shared<std::atomic<uint32_t>>(0);
    pending_notifier = std::make_unique<WaylandClientNotifier>(
        shell,
        session_authorizer,
        *registry,
        serial_source,
        [this](
            int socket_fd,
            std::shared_ptr<scene::Session> const& session,
            std::shared_ptr<wayland_rs::Client> const& client)
        {
            auto const handler_iter = connect_handlers.find(socket_fd);
            if (handler_iter != std::end(connect_handlers))
            {
                auto const callback = handler_iter->second;
                connect_handlers.erase(handler_iter);
                callback(session);
            }

            auto const client_handler_iter = client_connect_handlers.find(socket_fd);
            if (client_handler_iter != std::end(client_connect_handlers))
            {
                auto const callback = client_handler_iter->second;
                client_connect_handlers.erase(client_handler_iter);
                callback(client);
            }
        });

    // Run the builders before the server starts (because that's what GTK3
    // expects). There is no libwayland display in the wayland_rs backend, so
    // builders that need one get nullptr.
    extensions->run_builders(
        nullptr,
        [executor = this->executor](std::function<void()>&& work) { executor->spawn(std::move(work)); });

    extensions->init(WaylandExtensions::Context{
        nullptr,
        executor,
        shell,
        session_authorizer,
        main_clipboard,
        primary_selection_clipboard,
        text_input_hub,
        idle_hub,
        // TODO: WlSeat is now owned by the WaylandGlobalFactory; extensions that
        // need it (e.g. XWaylandWMShell) must obtain it once XWayland is ported
        // to the wayland_rs frontend.
        nullptr,
        output_manager.get(),
        surface_stack,
        input_device_registry,
        composite_event_filter,
        allocator,
        screen_shooter_factory,
        main_loop,
        desktop_file_manager,
        session_lock,
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

    wayland_display = choose_wayland_socket_name();

    if (arw_socket)
    {
        // The socket is bound on the event-loop thread inside run(); relax its
        // permissions once that has happened.
        auto const socket_name = wayland_display;
        executor->spawn(
            [socket_name]()
            {
                if (auto const runtime_dir = std::getenv("XDG_RUNTIME_DIR"))
                {
                    ::chmod((std::string{runtime_dir} + "/" + socket_name).c_str(),
                            S_IRUSR|S_IWUSR| S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);
                }
            });
    }
}

mf::WaylandConnector::~WaylandConnector()
{
    if (dispatch_thread.joinable())
    {
        server_wrapper->server->stop();
        dispatch_thread.join();
    }
}

void mf::WaylandConnector::start()
{
    dispatch_thread = std::thread{
        [this]()
        {
            mir::set_thread_name("Mir/Wayland");
            try
            {
                server_wrapper->server->run(
                    wayland_display,
                    std::move(pending_factory),
                    std::move(pending_notifier),
                    std::make_unique<WaylandWorkCallback>(executor));
            }
            catch (std::exception const& e)
            {
                mir::log_error("Wayland server terminated: %s", e.what());
            }
        }};
}

void mf::WaylandConnector::stop()
{
    server_wrapper->server->stop();
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
    int socket_fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, socket_fds) < 0)
        BOOST_THROW_EXCEPTION((std::system_error{
            errno, std::system_category(), "Failed to create socket pair for Wayland client"}));

    // The server end is adopted by the wayland_rs backend, which takes ownership
    // and closes it when the client disconnects; we must not close it here. The
    // client end is returned to (and owned by) the caller.
    server_wrapper->server->insert_client(socket_fds[0]);
    return socket_fds[1];
}

int mf::WaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<scene::Session> const& session)> const& connect_handler) const
{
    int socket_fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, socket_fds) < 0)
        BOOST_THROW_EXCEPTION((std::system_error{
            errno, std::system_category(), "Failed to create socket pair for Wayland client"}));

    auto const server_fd = socket_fds[0];

    // Register the connect handler and inject the client on the event-loop
    // thread: `connect_handlers` is only touched there, and registering the
    // handler before inserting the client guarantees it is present when the
    // notifier reports the new client (via on_client_connected, keyed by the
    // server-end fd the backend records for the injected socket).
    executor->spawn(
        [this, server_fd, connect_handler]()
        {
            connect_handlers[server_fd] = connect_handler;
            server_wrapper->server->insert_client(server_fd);
        });

    return socket_fds[1];
}

void mf::WaylandConnector::run_on_wayland_display(std::function<void(wl_display*)> const& /*functor*/)
{
    // TODO: there is no libwayland wl_display in the wayland_rs backend. Callers
    // that need to run work on the Wayland event loop should be ported to the
    // executor; XWayland integration is pending.
}

void mf::WaylandConnector::create_wayland_client(
    Fd const& client_fd,
    std::function<void(std::shared_ptr<wayland_rs::Client> const& client)> const& on_created)
{
    // `insert_client` takes ownership of the fd and closes it when the client
    // disconnects, so hand the backend a duplicate and keep our own Fd intact.
    auto const server_fd = ::dup(client_fd);
    if (server_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{
            errno, std::system_category(), "Failed to duplicate fd for Wayland client"}));

    // Register the client handler and inject the client on the event-loop
    // thread: `client_connect_handlers` is only touched there, and registering
    // the handler before inserting the client guarantees it is present when the
    // notifier reports the new client (keyed by the server-end fd the backend
    // records for the injected socket).
    executor->spawn(
        [this, server_fd, on_created]()
        {
            client_connect_handlers[server_fd] = on_created;
            server_wrapper->server->insert_client(server_fd);
        });
}

void mf::WaylandConnector::on_surface_created(
    wayland_rs::Client* /*client*/,
    uint32_t /*id*/,
    std::function<void(WlSurface*)> const& /*callback*/)
{
    // TODO: XWayland surface hook; pending XWayland port to the wayland_rs frontend.
}

auto mf::WaylandConnector::socket_name() const -> std::optional<std::string>
{
    if (wayland_display.empty())
        return std::nullopt;
    return wayland_display;
}

auto mf::WaylandConnector::get_extension(std::string const& name) const -> std::shared_ptr<void>
{
    return extensions->get_extension(name);
}

void mf::WaylandConnector::for_each_output_binding(
    wayland_rs::Client* /*client*/,
    graphics::DisplayConfigurationOutputId /*output*/,
    std::function<void(wl_resource* output)> const& /*callback*/)
{
    // TODO: the old libwayland wl_resource-based per-output binding bridge is not
    // available in the wayland_rs frontend. Consumers (display configuration /
    // XWayland) need porting to the new OutputManager binding model.
}

auto mir::frontend::get_session(wl_client const* /*wl_client*/) -> std::shared_ptr<scene::Session>
{
    // TODO: libwayland wl_client-based session lookup; pending XWayland port.
    return {};
}

auto mf::get_session(wl_resource* /*surface*/) -> std::shared_ptr<ms::Session>
{
    // TODO: libwayland wl_resource-based session lookup; pending XWayland port.
    return {};
}
