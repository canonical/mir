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

#include "add_virtual_device.h"

#include <miral/internal_client.h>
#include <miral/minimal_window_manager.h>
#include <miral/test_server.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/window_specification.h>

#include <mir/input/event_builder.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/server.h>
#include <mir/test/signal.h>

#include <wayland-client.h>

#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <vector>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;
using namespace mir::geometry;

namespace mir::wayland
{
extern struct wl_interface const xdg_wm_base_interface_data;
extern struct wl_interface const xdg_surface_interface_data;
extern struct wl_interface const xdg_toplevel_interface_data;
}

namespace
{
/// The stub display used by the headless test server.
Rectangle const output{{0, 0}, {1600, 1600}};

/// A restored window small enough that it cannot cover `pointer_outside_restored`.
Rectangle const restored_geometry{{100, 100}, {100, 100}};

/// Well away from `restored_geometry`, but inside the (fullscreen) output.
PointF const pointer_outside_restored{1200, 1200};

/// Inside `restored_geometry`, and so covered by the window in both states.
PointF const pointer_inside_restored{150, 150};

auto const event_timeout = 5s;

/// Request opcodes, from the xdg-shell protocol definition.
enum : uint32_t
{
    xdg_wm_base_get_xdg_surface = 2,
    xdg_surface_get_toplevel = 1,
    xdg_surface_ack_configure = 4,
    xdg_toplevel_set_fullscreen = 11,
    xdg_toplevel_unset_fullscreen = 12,
};

// Helper to avoid the need for C-style casts when adding a listener
auto wl_proxy_add_listener(struct wl_proxy *proxy, auto* impl, void *data)
{
    return wl_proxy_add_listener(
        proxy,
        reinterpret_cast<void(**)(void)>(const_cast<std::remove_const_t<std::remove_pointer_t<decltype(impl)>>*>(impl)),
        data);
}

enum class PointerEvent
{
    enter,
    leave
};

auto operator<<(std::ostream& out, PointerEvent event) -> std::ostream&
{
    return out << (event == PointerEvent::enter ? "enter" : "leave");
}

/// The latest wl_pointer enter/leave event the client saw
class PointerEventCache
{
public:
    void record(PointerEvent e)
    {
        event = e;
    }

    auto last_event() const
    {
        return event.load();
    }

    /// Ignore everything recorded so far, so a test can assert on one transition in isolation.
    void reset()
    {
        event = std::nullopt;
    }

private:

    std::atomic<std::optional<PointerEvent>> event;
};

/// A single xdg_toplevel with a wl_pointer, driven from the test thread by posting tasks onto
/// the client's own thread: all Wayland calls (requests and dispatch) happen there, so there
/// is no need to make libwayland calls from two threads.
struct PointerClient
{
    PointerEventCache event_cache;
    mir::test::Signal ready;

    operator std::function<void(wl_display*)>()
    {
        return [this](wl_display* display) { run_client(display); };
    }

    void set_fullscreen()
    {
        run_on_client_thread([this] { wl_proxy_marshal(toplevel, xdg_toplevel_set_fullscreen, nullptr); });
    }

    void unset_fullscreen()
    {
        run_on_client_thread([this] { wl_proxy_marshal(toplevel, xdg_toplevel_unset_fullscreen); });
    }

    void roundtrip()
    {
        run_on_client_thread([] {});
    }

    void stop()
    {
        {
            std::lock_guard lock{mutex};
            running = false;
        }
        cv.notify_one();
    }

private:

    void run_client(wl_display* display)
    {
        this->display = display;

        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        if (!compositor || !shm || !seat || !wm_base)
        {
            ADD_FAILURE() << "required Wayland globals are missing";
            ready.raise();
            return;
        }

        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, this);

        surface = wl_compositor_create_surface(compositor);

        xdg_surface = wl_proxy_marshal_constructor(
            wm_base, xdg_wm_base_get_xdg_surface, &mir::wayland::xdg_surface_interface_data, nullptr, surface);
        wl_proxy_add_listener(xdg_surface, &xdg_surface_listener, this);

        toplevel = wl_proxy_marshal_constructor(
            xdg_surface, xdg_surface_get_toplevel, &mir::wayland::xdg_toplevel_interface_data);
        wl_proxy_add_listener(toplevel, &xdg_toplevel_listener, this);

        create_pool();

        wl_surface_commit(surface);
        wl_display_roundtrip(display);

        // Mir only creates the scene surface (and so the window) once content is attached.
        attach_buffer();
        wl_display_roundtrip(display);

        ready.raise();
        dispatch_until_stopped();
    }

    /// Runs `task` on the client thread, returning once it (and the resulting round-trip)
    /// has completed.
    void run_on_client_thread(std::function<void()>&& task)
    {
        auto const done = std::make_shared<mir::test::Signal>();
        {
            std::lock_guard lock{mutex};
            tasks.push_back(
                [this, done, task = std::move(task)]
                {
                    task();
                    wl_display_roundtrip(display);

                    wl_display_flush(display);

                    pollfd fd{wl_display_get_fd(display), POLLIN, 0};
                    if (poll(&fd, 1, 10) > 0)
                        wl_display_dispatch(display);
                    else
                        wl_display_dispatch_pending(display);
                    done->raise();
                });
        }
        cv.notify_one();
        EXPECT_TRUE(done->wait_for(event_timeout));
    }

    void dispatch_until_stopped()
    {
        while (true)
        {
            std::vector<std::function<void()>> pending;
            {
                std::unique_lock lock{mutex};
                cv.wait_for(lock, 10ms, [this]() { return !tasks.empty() || !running; });
                if (!running)
                    return;
                pending.swap(tasks);
            }

            for (auto const& task : pending)
                task();
        }
    }

    void create_pool()
    {
        auto const size = output.size.width.as_int() * output.size.height.as_int() * 4;
        auto const fd = memfd_create("mir-test-buffer", MFD_CLOEXEC);
        EXPECT_THAT(fd, Ge(0));
        EXPECT_THAT(ftruncate(fd, size), Eq(0));
        pool = wl_shm_create_pool(shm, fd, size);
        close(fd);
    }

    /// The input region follows the committed buffer, so every configure needs a matching commit.
    void attach_buffer()
    {
        auto const stride = width * 4;
        auto* const buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<PointerClient*>(data);

        if (std::strcmp(interface, wl_compositor_interface.name) == 0)
            self->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 1));
        else if (std::strcmp(interface, wl_shm_interface.name) == 0)
            self->shm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        else if (std::strcmp(interface, wl_seat_interface.name) == 0)
            self->seat =
                static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, std::min(version, 5u)));
        else if (std::strcmp(interface, mir::wayland::xdg_wm_base_interface_data.name) == 0)
            self->wm_base =
                static_cast<wl_proxy*>(wl_registry_bind(registry, id, &mir::wayland::xdg_wm_base_interface_data, 1));
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static void handle_xdg_surface_configure(void* data, wl_proxy* xdg_surface, uint32_t serial)
    {
        auto* const self = static_cast<PointerClient*>(data);
        wl_proxy_marshal(xdg_surface, xdg_surface_ack_configure, serial);
        if (self->surface)
            self->attach_buffer();
    }

    static void handle_toplevel_configure(void* data, wl_proxy*, int32_t width, int32_t height, wl_array*)
    {
        auto* const self = static_cast<PointerClient*>(data);
        if (width > 0 && height > 0)
        {
            self->width = width;
            self->height = height;
        }
    }

    static void handle_toplevel_close(void*, wl_proxy*) {}

    static void handle_enter(void* data, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t /*x*/, wl_fixed_t /*y*/)
    {
        static_cast<PointerClient*>(data)->event_cache.record(PointerEvent::enter);
    }

    static void handle_move(void* /*data*/, wl_pointer*, uint32_t, wl_fixed_t /*x*/, wl_fixed_t /*y*/)
    {
    }

    static void handle_leave(void* data, wl_pointer*, uint32_t, wl_surface*)
    {
        static_cast<PointerClient*>(data)->event_cache.record(PointerEvent::leave);
    }

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};

    static inline struct
    {
        void (*configure)(void *data, wl_proxy *xdg_surface, uint32_t serial);
    } constexpr xdg_surface_listener{ .configure = &handle_xdg_surface_configure};

    static inline struct
    {
        void (*configure)(void *data, wl_proxy *xdg_toplevel, int32_t width, int32_t height, wl_array *states);
        void (*close)(void *data, wl_proxy *xdg_toplevel);
    } constexpr xdg_toplevel_listener{ .configure = &handle_toplevel_configure, .close = &handle_toplevel_close };

    /// Only the events up to wl_pointer version 5 can arrive, as that is the version bound above.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    static inline wl_pointer_listener const pointer_listener{
        .enter = &handle_enter,
        .leave = &handle_leave,
        .motion = &handle_move,
        .button = [](void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t) {},
        .axis = [](void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {},
        .frame = [](void*, wl_pointer*) {},
        .axis_source = [](void*, wl_pointer*, uint32_t) {},
        .axis_stop = [](void*, wl_pointer*, uint32_t, uint32_t) {},
        .axis_discrete = [](void*, wl_pointer*, uint32_t, int32_t) {},
        .axis_value120 = [](void*, wl_pointer*, uint32_t, int32_t) {},
        .axis_relative_direction = [](void*, wl_pointer*, uint32_t, uint32_t) {}};
#pragma GCC diagnostic pop

    wl_display* display{nullptr};
    wl_compositor* compositor{nullptr};
    wl_shm* shm{nullptr};
    wl_shm_pool* pool{nullptr};
    wl_seat* seat{nullptr};
    wl_pointer* pointer{nullptr};
    wl_surface* surface{nullptr};
    wl_proxy* wm_base{nullptr};
    wl_proxy* xdg_surface{nullptr};
    wl_proxy* toplevel{nullptr};
    int width{restored_geometry.size.width.as_int()};
    int height{restored_geometry.size.height.as_int()};

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::function<void()>> tasks;
    bool running{true};
};

wl_registry_listener constexpr PointerClient::registry_listener;

/// Lets the test wait for the client's window, which appears asynchronously on the server thread.
struct WindowRecord
{
    void record(miral::Window const& window)
    {
        std::lock_guard lock{mutex};
        window_ = window;
        cv.notify_all();
    }

    auto wait_for_window() -> std::optional<miral::Window>
    {
        std::unique_lock lock{mutex};
        if (!cv.wait_for(lock, event_timeout, [this] { return !!window_; }))
            return std::nullopt;
        return window_;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    miral::Window window_;
};

struct RecordingPolicy : miral::MinimalWindowManager
{
    RecordingPolicy(miral::WindowManagerTools const& tools, std::shared_ptr<WindowRecord> record) :
        MinimalWindowManager{tools}, record{std::move(record)}
    {
    }

    void advise_new_window(miral::WindowInfo const& window_info) override
    {
        MinimalWindowManager::advise_new_window(window_info);
        record->record(window_info.window());
    }

    std::shared_ptr<WindowRecord> const record;
};
}

/// Fullscreening (or restoring) a window changes what is under the pointer without the pointer
/// moving: the client must be told, or it is left believing the pointer is somewhere it isn't.
struct PointerFullscreen : miral::TestServer
{
    PointerFullscreen()
    {
        start_server_in_setup = false;

        add_server_init(
            [this](mir::Server& server)
            {
                server.add_init_callback(
                    [&server, this]
                    {
                        input_device_registry = server.the_input_device_registry();
                        input_device_hub = server.the_input_device_hub();
                    });
            });

        add_server_init(launcher);
    }

    auto build_window_manager_policy(miral::WindowManagerTools const& tools)
        -> std::unique_ptr<miral::WindowManagementPolicy> override
    {
        return std::make_unique<RecordingPolicy>(tools, record);
    }

    void TearDown() override
    {
        pointer_client.stop();
        miral::TestServer::TearDown();
    }

    /// Starts the server, launches the client, waits for its window and places it at
    /// `restored_geometry`.
    void start_server_and_client()
    {
        start_server();

        launcher.launch(pointer_client, [](auto){});
        ASSERT_TRUE(pointer_client.ready.wait_for(event_timeout));

        window = record->wait_for_window();
        ASSERT_TRUE(window);

        invoke_tools(
            [this](miral::WindowManagerTools& tools)
            {
                miral::WindowSpecification spec;
                spec.top_left() = restored_geometry.top_left;
                spec.size() = restored_geometry.size;
                tools.modify_window(tools.info_for(*window), spec);
            });

        pointer_device = miral::test::add_test_device(
            input_device_registry.lock(), input_device_hub.lock(), mir::input::DeviceCapability::pointer);
    }

    void move_pointer_to(PointF position)
    {
        pointer_device->if_started_then(
            [position](auto* sink, auto* builder)
            {
                sink->handle_input(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_motion,
                    MirPointerButtons{0},
                    position,
                    DisplacementF{0, 0},
                    mir_pointer_axis_source_none,
                    {},
                    {}));
            });

        pointer_client.roundtrip();
    }

    PointerClient pointer_client;

protected:
    std::shared_ptr<WindowRecord> const record{std::make_shared<WindowRecord>()};
    miral::InternalClientLauncher launcher;
    std::optional<miral::Window> window;
    std::shared_ptr<mir::input::VirtualInputDevice> pointer_device;
    std::weak_ptr<mir::input::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub;
};

TEST_F(PointerFullscreen, cursor_enters_surface_when_fullscreen_grows_under_the_pointer)
{
    start_server_and_client();
    move_pointer_to(pointer_outside_restored);
    EXPECT_THAT(pointer_client.event_cache.last_event(), Ne(PointerEvent::enter));

    pointer_client.set_fullscreen();

    EXPECT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));
}

TEST_F(PointerFullscreen, cursor_leaves_surface_when_restore_moves_it_away_from_the_pointer)
{
    start_server_and_client();

    move_pointer_to(pointer_outside_restored);
    pointer_client.set_fullscreen();
    ASSERT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));

    pointer_client.event_cache.reset();
    pointer_client.unset_fullscreen();

    EXPECT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::leave));
}

TEST_F(PointerFullscreen, cursor_stays_in_surface_when_fullscreening_under_the_pointer)
{
    start_server_and_client();

    move_pointer_to(pointer_inside_restored);

    ASSERT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));

    pointer_client.set_fullscreen();

    // The surface is under the pointer both before and after, so the client must not be told it left.
    EXPECT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));
}

TEST_F(PointerFullscreen, cursor_stays_in_surface_when_restoring_under_the_pointer)
{
    start_server_and_client();

    move_pointer_to(pointer_inside_restored);
    pointer_client.set_fullscreen();
    ASSERT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));

    pointer_client.unset_fullscreen();

    EXPECT_THAT(pointer_client.event_cache.last_event(), Eq(PointerEvent::enter));
}
