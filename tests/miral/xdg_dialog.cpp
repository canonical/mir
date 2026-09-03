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
#include <cstring>
#include <mutex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

/// The type Mir gives a Wayland toplevel that has not asked to be anything else.
auto constexpr default_window_type = mir_window_type_freestyle;
auto constexpr client_completion_timeout = std::chrono::seconds{2};

namespace mir::wayland
{
extern struct wl_interface const wl_compositor_interface_data;
extern struct wl_interface const wl_surface_interface_data;
extern struct wl_interface const wl_shm_interface_data;
extern struct wl_interface const xdg_wm_base_interface_data;
extern struct wl_interface const xdg_surface_interface_data;
extern struct wl_interface const xdg_toplevel_interface_data;
extern struct wl_interface const xdg_wm_dialog_v1_interface_data;
extern struct wl_interface const xdg_dialog_v1_interface_data;
}

namespace
{
/// Request opcodes, from the xdg-shell and xdg-dialog-v1 protocol definitions.
enum : uint32_t
{
    xdg_wm_base_get_xdg_surface = 2,
    xdg_surface_get_toplevel = 1,
    xdg_surface_ack_configure = 4,
    xdg_wm_dialog_v1_get_xdg_dialog = 1,
    xdg_dialog_v1_set_modal = 1,
    xdg_dialog_v1_unset_modal = 2,
};

/// Adapts a callable into the pair of callbacks miral::InternalClientLauncher expects.
class WaylandClient
{
public:
    void operator()(struct wl_display* display) { code(display); }

    void operator()(std::weak_ptr<mir::scene::Session> const&) {}

    std::function<void(struct wl_display*)> code{[](auto) {}};
};

/// Creates a single xdg_toplevel with an associated xdg_dialog_v1, driving the requests
/// under test before the first commit — the order GTK4 uses, and the order in which a
/// window type change can still take effect (see `miral::WindowInfo::can_morph_to()`).
struct DialogClient
{
    /// Called with the freshly created xdg_dialog_v1, before the toplevel is committed.
    std::function<void(wl_proxy* dialog)> before_commit{[](auto) {}};

    void operator()(wl_display* display)
    {
        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(wm_base, NotNull());
        ASSERT_THAT(wm_dialog, NotNull());

        auto* const surface = wl_proxy_marshal_constructor(
            reinterpret_cast<wl_proxy*>(compositor), 0, &mir::wayland::wl_surface_interface_data, nullptr);

        auto* const xdg_surface = wl_proxy_marshal_constructor(
            wm_base, xdg_wm_base_get_xdg_surface, &mir::wayland::xdg_surface_interface_data, nullptr, surface);

        auto* const toplevel = wl_proxy_marshal_constructor(
            xdg_surface, xdg_surface_get_toplevel, &mir::wayland::xdg_toplevel_interface_data);

        wl_proxy_add_listener(xdg_surface, xdg_surface_listener, this);

        auto* const dialog = wl_proxy_marshal_constructor(
            wm_dialog,
            xdg_wm_dialog_v1_get_xdg_dialog,
            &mir::wayland::xdg_dialog_v1_interface_data,
            nullptr,
            toplevel);

        before_commit(dialog);

        // This client runs inside the server, so it must never block waiting for events:
        // round-trips let the configure arrive (and be acked by the listener) without
        // stalling the compositor.
        wl_surface_commit(reinterpret_cast<wl_surface*>(surface));
        wl_display_roundtrip(display);
        ASSERT_TRUE(configured);

        // Mir only creates the scene surface (and so the window the test inspects) once
        // the toplevel has content attached.
        attach_buffer(display, reinterpret_cast<wl_surface*>(surface));
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<DialogClient*>(data);

        auto bind = [&](wl_interface const& iface, auto& out)
        {
            if (std::strcmp(interface, iface.name) == 0)
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    wl_registry_bind(registry, id, &iface, version));
        };

        bind(mir::wayland::wl_compositor_interface_data, self->compositor);
        bind(mir::wayland::wl_shm_interface_data, self->shm);
        bind(mir::wayland::xdg_wm_base_interface_data, self->wm_base);
        bind(mir::wayland::xdg_wm_dialog_v1_interface_data, self->wm_dialog);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static void handle_configure(void* data, wl_proxy* xdg_surface, uint32_t serial)
    {
        wl_proxy_marshal(xdg_surface, xdg_surface_ack_configure, serial);
        static_cast<DialogClient*>(data)->configured = true;
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

    wl_compositor* compositor{nullptr};
    wl_shm* shm{nullptr};
    wl_proxy* wm_base{nullptr};
    wl_proxy* wm_dialog{nullptr};
    bool configured{false};
};

wl_registry_listener constexpr DialogClient::registry_listener;

/// Lets the test wait for the client's window to appear, since it is created
/// asynchronously on the server thread.
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

struct XdgDialog : miral::TestServer
{
    XdgDialog()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    auto build_window_manager_policy(miral::WindowManagerTools const& tools)
        -> std::unique_ptr<miral::WindowManagementPolicy> override
    {
        return std::make_unique<RecordingPolicy>(tools, record);
    }

    /// Maps a toplevel that has an xdg_dialog_v1 attached, running `before_commit` against
    /// that dialog first, and returns the type of the resulting window.
    ///
    /// The type is sampled while the client is still connected, because the client's
    /// windows are destroyed as soon as it goes away. Nullopt means no window appeared;
    /// returning it lets the caller assert on the test thread, as gtest assertions on the
    /// client and server threads are unreliable.
    auto map_toplevel_with_dialog(std::function<void(wl_proxy* dialog)>&& before_commit)
        -> std::optional<MirWindowType>
    {
        mir::test::Signal done;
        std::optional<MirWindowType> type;
        DialogClient dialog_client;
        dialog_client.before_commit = std::move(before_commit);

        client.code = [&](wl_display* display)
        {
            dialog_client(display);

            if (auto const window = record->wait_for_window())
                invoke_tools([&](miral::WindowManagerTools& tools) { type = tools.info_for(*window).type(); });

            done.raise();
        };

        launcher.launch(client);
        EXPECT_TRUE(done.wait_for(client_completion_timeout));

        return type;
    }

private:
    std::shared_ptr<WindowRecord> const record{std::make_shared<WindowRecord>()};
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};
}

// GTK4 creates an xdg_dialog_v1 for ordinary main windows and immediately unsets modality,
// so merely creating the object must not make the window a dialog: dialogs are excluded
// from the foreign-toplevel protocols, so retyping dropped the window out of taskbars and
// app switchers while it was still on screen.
TEST_F(XdgDialog, creating_dialog_object_does_not_make_window_a_dialog)
{
    start_server();

    EXPECT_THAT(map_toplevel_with_dialog([](auto) {}), Optional(Eq(default_window_type)));
}

TEST_F(XdgDialog, set_modal_makes_window_a_dialog)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_with_dialog([](wl_proxy* dialog) { wl_proxy_marshal(dialog, xdg_dialog_v1_set_modal); }),
        Optional(Eq(mir_window_type_dialog)));
}

// The type in force before set_modal() must be restored, rather than the window being
// forced to `normal` — a toplevel may carry a mir-shell archetype such as `satellite`.
TEST_F(XdgDialog, unset_modal_restores_the_type_from_before_set_modal)
{
    start_server();

    auto const type = map_toplevel_with_dialog(
        [](wl_proxy* dialog)
        {
            wl_proxy_marshal(dialog, xdg_dialog_v1_set_modal);
            wl_proxy_marshal(dialog, xdg_dialog_v1_unset_modal);
        });

    EXPECT_THAT(type, Optional(Eq(default_window_type)));
}

// GTK4 calls unset_modal() on a toplevel that was never made modal; that must leave the
// window's type alone rather than forcing it to `normal`.
TEST_F(XdgDialog, unset_modal_without_set_modal_leaves_type_unchanged)
{
    start_server();

    EXPECT_THAT(
        map_toplevel_with_dialog([](wl_proxy* dialog) { wl_proxy_marshal(dialog, xdg_dialog_v1_unset_modal); }),
        Optional(Eq(default_window_type)));
}
