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

#include <miral/test_server.h>

#include <miral/internal_client.h>
#include <miral/minimal_window_manager.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

#include <mir/test/signal.h>

#include <wayland-client.h>

#include <sys/mman.h>
#include <unistd.h>

#include <condition_variable>
#include <chrono>
#include <optional>
#include <string_view>
#include <mutex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

auto constexpr client_completion_timeout = std::chrono::seconds{6};

namespace mir::wayland
{
extern struct wl_interface const wl_compositor_interface_data;
extern struct wl_interface const wl_surface_interface_data;
extern struct wl_interface const wl_shm_interface_data;
extern struct wl_interface const xdg_wm_base_interface_data;
extern struct wl_interface const xdg_surface_interface_data;
extern struct wl_interface const xdg_toplevel_interface_data;
extern struct wl_interface const mir_shell_v1_interface_data;
extern struct wl_interface const mir_positioner_v1_interface_data;
extern struct wl_interface const mir_regular_surface_v1_interface_data;
extern struct wl_interface const mir_floating_regular_surface_v1_interface_data;
extern struct wl_interface const mir_dialog_surface_v1_interface_data;
}

namespace
{
/// Request opcodes, from the xdg-shell and mir-shell-unstable-v1 protocol definitions.
enum : uint32_t
{
    wl_compositor_create_surface = 0,
    xdg_wm_base_get_xdg_surface = 2,
    xdg_surface_get_toplevel = 1,
    xdg_surface_ack_configure = 4,
    mir_shell_v1_get_regular_surface = 0,
    mir_shell_v1_get_floating_regular_surface = 1,
    mir_shell_v1_get_dialog_surface = 2,
};

/// The properties of the window an archetype produced.
struct WindowProperties
{
    MirWindowType type;
    MirDepthLayer depth_layer;

    auto operator==(WindowProperties const&) const -> bool = default;
};

auto operator<<(std::ostream& out, WindowProperties const& properties) -> std::ostream&
{
    return out << "{type " << properties.type << ", depth layer " << properties.depth_layer << "}";
}

/// Adapts a callable into the pair of callbacks miral::InternalClientLauncher expects.
class WaylandClient
{
public:
    void operator()(struct wl_display* display) { code(display); }

    void operator()(std::weak_ptr<mir::scene::Session> const&) {}

    std::function<void(struct wl_display*)> code{[](auto) {}};
};

/// Maps a single xdg_toplevel, assigning it an archetype during the initial commit without a
/// buffer attached — the sequence mir_shell_v1 tells clients to use.
struct ArchetypeClient
{
    /// Called with the mir_shell_v1 and the wl_surface, before the toplevel is committed.
    std::function<void(wl_proxy* mir_shell, wl_proxy* surface)> assign_archetype{[](auto, auto) {}};

    /// Called with the same arguments once the toplevel has been mapped.
    std::function<void(wl_proxy* mir_shell, wl_proxy* surface)> assign_archetype_after_map{[](auto, auto) {}};

    void operator()(wl_display* display)
    {
        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(wm_base, NotNull());
        ASSERT_THAT(mir_shell, NotNull());

        auto* const surface = wl_proxy_marshal_constructor(
            compositor, wl_compositor_create_surface, &mir::wayland::wl_surface_interface_data, nullptr);

        auto* const xdg_surface = wl_proxy_marshal_constructor(
            wm_base, xdg_wm_base_get_xdg_surface, &mir::wayland::xdg_surface_interface_data, nullptr, surface);

        wl_proxy_marshal_constructor(xdg_surface, xdg_surface_get_toplevel, &mir::wayland::xdg_toplevel_interface_data);

        wl_proxy_add_listener(xdg_surface, xdg_surface_listener, this);

        assign_archetype(mir_shell, surface);

        // This client runs inside the server, so it must never block waiting for events.
        wl_surface_commit(reinterpret_cast<wl_surface*>(surface));
        wl_display_roundtrip(display);
        ASSERT_TRUE(configured);

        // Mir only creates the scene surface (and so the window the test inspects) once the
        // toplevel has content attached.
        attach_buffer(display, reinterpret_cast<wl_surface*>(surface));

        assign_archetype_after_map(mir_shell, surface);
        wl_surface_commit(reinterpret_cast<wl_surface*>(surface));
        wl_display_roundtrip(display);
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<ArchetypeClient*>(data);

        auto bind = [&](wl_interface const& iface, auto& out)
        {
            if (std::string_view(interface) == std::string_view(iface.name))
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    wl_registry_bind(registry, id, &iface, version));
        };

        bind(mir::wayland::wl_compositor_interface_data, self->compositor);
        bind(mir::wayland::wl_shm_interface_data, self->shm);
        bind(mir::wayland::xdg_wm_base_interface_data, self->wm_base);
        bind(mir::wayland::mir_shell_v1_interface_data, self->mir_shell);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static void handle_configure(void* data, wl_proxy* xdg_surface, uint32_t serial)
    {
        wl_proxy_marshal(xdg_surface, xdg_surface_ack_configure, serial);
        static_cast<ArchetypeClient*>(data)->configured = true;
    }

    void attach_buffer(wl_display* display, wl_surface* surface)
    {
        ASSERT_THAT(shm, NotNull());

        auto const stride = width * 4;
        auto const size = stride * height;

        auto const fd = memfd_create("mir-test-buffer", MFD_CLOEXEC);
        ASSERT_THAT(fd, Ge(0));
        ASSERT_THAT(ftruncate(fd, size), Eq(0));

        auto* const pool = wl_shm_create_pool(shm, fd, size);
        auto* const buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
        wl_display_roundtrip(display);
    }

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};

    /// xdg_surface has a single event, `configure`.
    static inline void (*xdg_surface_listener[])(){reinterpret_cast<void (*)()>(&handle_configure)};

    static int constexpr width = 100;
    static int constexpr height = 100;

    wl_proxy* compositor{nullptr};
    wl_shm* shm{nullptr};
    wl_proxy* wm_base{nullptr};
    wl_proxy* mir_shell{nullptr};
    bool configured{false};
};

/// Lets the test wait for the client's window to appear, since it is created asynchronously on
/// the server thread.
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
        if (!cv.wait_for(lock, std::chrono::seconds{5}, [this] { return window_; }))
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
    RecordingPolicy(miral::WindowManagerTools const& tools, std::shared_ptr<WindowRecord> record)
        : MinimalWindowManager{tools}, record{std::move(record)}
    {
    }

    void advise_new_window(miral::WindowInfo const& window_info) override
    {
        MinimalWindowManager::advise_new_window(window_info);
        record->record(window_info.window());
    }

    std::shared_ptr<WindowRecord> const record;
};

struct MirShellArchetypes : miral::TestServer
{
    MirShellArchetypes()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    auto build_window_manager_policy(miral::WindowManagerTools const& tools)
        -> std::unique_ptr<miral::WindowManagementPolicy> override
    {
        return std::make_unique<RecordingPolicy>(tools, record);
    }

    /// Maps a toplevel carrying the archetype `assign_archetype` gives it, and returns the
    /// properties of the resulting window.
    ///
    /// The properties are sampled while the client is still connected, because the client's
    /// windows are destroyed as soon as it goes away. Nullopt means no window appeared;
    /// returning it lets the caller assert on the test thread, as gtest assertions on the
    /// client and server threads are unreliable.
    auto map_toplevel_with_archetype(std::function<void(wl_proxy* mir_shell, wl_proxy* surface)>&& assign_archetype)
        -> std::optional<WindowProperties>
    {
        return map_toplevel(std::move(assign_archetype), [](auto, auto) {});
    }

    /// As above, but the archetype is assigned once the window has been mapped.
    auto map_toplevel_then_assign_archetype(
        std::function<void(wl_proxy* mir_shell, wl_proxy* surface)>&& assign_archetype)
        -> std::optional<WindowProperties>
    {
        return map_toplevel([](auto, auto) {}, std::move(assign_archetype));
    }

private:
    auto map_toplevel(
        std::function<void(wl_proxy* mir_shell, wl_proxy* surface)>&& assign_archetype,
        std::function<void(wl_proxy* mir_shell, wl_proxy* surface)>&& assign_archetype_after_map)
        -> std::optional<WindowProperties>
    {
        mir::test::Signal done;
        std::optional<WindowProperties> properties;
        ArchetypeClient archetype_client;
        archetype_client.assign_archetype = std::move(assign_archetype);
        archetype_client.assign_archetype_after_map = std::move(assign_archetype_after_map);

        client.code = [&](wl_display* display)
        {
            archetype_client(display);

            if (auto const window = record->wait_for_window())
                invoke_tools(
                    [&](miral::WindowManagerTools& tools)
                    {
                        auto const& info = tools.info_for(*window);
                        properties = WindowProperties{info.type(), info.depth_layer()};
                    });

            done.raise();
        };

        launcher.launch(client);
        EXPECT_TRUE(done.wait_for(client_completion_timeout));

        return properties;
    }

    std::shared_ptr<WindowRecord> const record{std::make_shared<WindowRecord>()};
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};
}

TEST_F(MirShellArchetypes, regular_surface_is_a_normal_window)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_with_archetype(
            [](wl_proxy* mir_shell, wl_proxy* surface)
            {
                wl_proxy_marshal_constructor(
                    mir_shell,
                    mir_shell_v1_get_regular_surface,
                    &mir::wayland::mir_regular_surface_v1_interface_data,
                    nullptr,
                    surface);
            }),
        Optional(Eq(WindowProperties{mir_window_type_normal, mir_depth_layer_application})));
}

// "A floating regular surface is 'always on top' of other application windows"
TEST_F(MirShellArchetypes, floating_regular_surface_is_a_utility_window_that_is_always_on_top)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_with_archetype(
            [](wl_proxy* mir_shell, wl_proxy* surface)
            {
                wl_proxy_marshal_constructor(
                    mir_shell,
                    mir_shell_v1_get_floating_regular_surface,
                    &mir::wayland::mir_floating_regular_surface_v1_interface_data,
                    nullptr,
                    surface);
            }),
        Optional(Eq(WindowProperties{mir_window_type_utility, mir_depth_layer_always_on_top})));
}

TEST_F(MirShellArchetypes, dialog_surface_is_a_dialog_window)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_with_archetype(
            [](wl_proxy* mir_shell, wl_proxy* surface)
            {
                wl_proxy_marshal_constructor(
                    mir_shell,
                    mir_shell_v1_get_dialog_surface,
                    &mir::wayland::mir_dialog_surface_v1_interface_data,
                    nullptr,
                    surface);
            }),
        Optional(Eq(WindowProperties{mir_window_type_dialog, mir_depth_layer_application})));
}

// "a client may, for transitions the compositor allows, assign a new archetype to a surface
// which already has one": retyping a mapped toplevel must work, rather than being rejected as
// an invalid type change (which reached the client as an internal error that disconnected it).
TEST_F(MirShellArchetypes, archetype_assigned_after_mapping_retypes_the_window)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_then_assign_archetype(
            [](wl_proxy* mir_shell, wl_proxy* surface)
            {
                wl_proxy_marshal_constructor(
                    mir_shell,
                    mir_shell_v1_get_floating_regular_surface,
                    &mir::wayland::mir_floating_regular_surface_v1_interface_data,
                    nullptr,
                    surface);
            }),
        Optional(Eq(WindowProperties{mir_window_type_utility, mir_depth_layer_always_on_top})));
}
