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

#include <mir/test/signal.h>

#include <wayland-client.h>

#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <string_view>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

auto constexpr client_completion_timeout = std::chrono::seconds{6};

namespace mir::wayland
{
extern struct wl_interface const wl_compositor_interface_data;
extern struct wl_interface const wl_surface_interface_data;
extern struct wl_interface const wl_subcompositor_interface_data;
extern struct wl_interface const wl_subsurface_interface_data;
extern struct wl_interface const wl_shm_interface_data;
}

namespace
{
/// Request opcodes, from the core Wayland protocol definition.
enum : uint32_t
{
    wl_compositor_create_surface = 0,
    wl_subcompositor_get_subsurface = 1,
    wl_subsurface_set_desync = 5,
};

/// Adapts a callable into the pair of callbacks miral::InternalClientLauncher expects.
class WaylandClient
{
public:
    void operator()(struct wl_display* display) { code(display); }

    void operator()(std::weak_ptr<mir::scene::Session> const&) {}

    std::function<void(struct wl_display*)> code{[](auto) {}};
};

/// A buffer, and whether the compositor has released it.
struct Buffer
{
    wl_buffer* buffer{nullptr};
    bool released{false};
};

/// Creates a surface with a sub-surface, which is synchronized as all new sub-surfaces
/// are, and commits to the sub-surface without ever committing the parent.
struct SubsurfaceClient
{
    /// Called with the sub-surface's wl_surface once it has been created.
    std::function<void(SubsurfaceClient& client, wl_surface* child)> commit_buffers{[](auto&, auto) {}};

    /// If set, the client keeps processing events until this returns true or it has waited
    /// for as long as an event that is coming could take.
    std::function<bool()> wait_for;

    void operator()(wl_display* display)
    {
        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        wl_display_roundtrip(display);

        if (!compositor || !subcompositor || !shm)
            return;

        auto* const parent = wl_proxy_marshal_constructor(
            compositor, wl_compositor_create_surface, &mir::wayland::wl_surface_interface_data, nullptr);
        auto* const child = wl_proxy_marshal_constructor(
            compositor, wl_compositor_create_surface, &mir::wayland::wl_surface_interface_data, nullptr);
        subsurface = wl_proxy_marshal_constructor(
            subcompositor,
            wl_subcompositor_get_subsurface,
            &mir::wayland::wl_subsurface_interface_data,
            nullptr,
            child,
            parent);

        commit_buffers(*this, reinterpret_cast<wl_surface*>(child));

        // This client runs inside the server, so it must never block waiting for events:
        // round-trips let any events arrive without stalling the compositor.
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        if (wait_for)
        {
            auto const give_up = std::chrono::steady_clock::now() + event_timeout;
            while (!wait_for() && std::chrono::steady_clock::now() < give_up)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
                wl_display_roundtrip(display);
            }
        }

        bound_globals = true;
    }

    /// Creates a buffer whose release is recorded in `buffer`.
    void create_buffer(Buffer& buffer)
    {
        auto const stride = width * 4;
        auto const size = stride * height;

        auto const fd = memfd_create("mir-test-buffer", MFD_CLOEXEC);
        if (fd < 0 || ftruncate(fd, size) != 0)
            return;

        auto* const pool = wl_shm_create_pool(shm, fd, size);
        buffer.buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        wl_buffer_add_listener(buffer.buffer, &buffer_listener, &buffer);
    }

    /// Asks to be told when it is a good time to draw `surface`, and commits that request.
    /// `done` is set when the compositor says so.
    static void commit_frame_request(wl_surface* surface, bool& done)
    {
        wl_callback_add_listener(wl_surface_frame(surface), &frame_listener, &done);
        wl_surface_commit(surface);
    }

    void set_desync() { wl_proxy_marshal(subsurface, wl_subsurface_set_desync); }

    static void commit(wl_surface* surface, Buffer const& buffer)
    {
        wl_surface_attach(surface, buffer.buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
    }

    static void new_global(void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<SubsurfaceClient*>(data);

        auto bind = [&](wl_interface const& iface, auto& out)
        {
            if (std::string_view(interface) == std::string_view(iface.name))
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    wl_registry_bind(registry, id, &iface, std::min(version, static_cast<uint32_t>(iface.version))));
        };

        bind(mir::wayland::wl_compositor_interface_data, self->compositor);
        bind(mir::wayland::wl_subcompositor_interface_data, self->subcompositor);
        bind(mir::wayland::wl_shm_interface_data, self->shm);
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static void buffer_released(void* data, wl_buffer*) { static_cast<Buffer*>(data)->released = true; }

    static void frame_done(void* data, wl_callback*, uint32_t) { *static_cast<bool*>(data) = true; }

    static wl_registry_listener constexpr registry_listener{&new_global, &global_remove};
    static wl_buffer_listener constexpr buffer_listener{&buffer_released};
    static wl_callback_listener constexpr frame_listener{&frame_done};

    /// Frame requests committed without a buffer are answered from a timer, so this only
    /// has to be long enough for that to have happened.
    static auto constexpr event_timeout = std::chrono::milliseconds{500};

    static int constexpr width = 100;
    static int constexpr height = 100;

    wl_proxy* compositor{nullptr};
    wl_proxy* subcompositor{nullptr};
    wl_proxy* subsurface{nullptr};
    wl_shm* shm{nullptr};

    /// Whether the client got as far as committing its buffers.
    bool bound_globals{false};
};

wl_registry_listener constexpr SubsurfaceClient::registry_listener;
wl_buffer_listener constexpr SubsurfaceClient::buffer_listener;
wl_callback_listener constexpr SubsurfaceClient::frame_listener;

struct Subsurface : miral::TestServer
{
    Subsurface()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    /// Runs a client that commits to a synchronized sub-surface, and waits for it to
    /// finish. Returns whether it did.
    ///
    /// Assertions are left to the caller, on the test thread, as gtest assertions on the
    /// client and server threads are unreliable.
    auto run(
        std::function<void(SubsurfaceClient& client, wl_surface* child)>&& commit_buffers,
        std::function<bool()>&& wait_for = {}) -> bool
    {
        mir::test::Signal done;
        SubsurfaceClient subsurface_client;
        subsurface_client.commit_buffers = std::move(commit_buffers);
        subsurface_client.wait_for = std::move(wait_for);

        client.code = [&](wl_display* display)
        {
            subsurface_client(display);
            done.raise();
        };

        launcher.launch(client);
        return done.wait_for(client_completion_timeout) && subsurface_client.bound_globals;
    }

private:
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};
}

// A buffer committed to a synchronized sub-surface is cached until the parent surface is
// committed. One that is replaced before then is never shown, but the client is still owed
// its release: EGL implementations keep a buffer locked until it arrives, and block in
// eglSwapBuffers() once every buffer of the surface is locked.
TEST_F(Subsurface, buffer_replaced_while_synchronized_is_released)
{
    start_server();

    Buffer first;
    Buffer second;

    ASSERT_TRUE(run([&](SubsurfaceClient& client, wl_surface* child)
        {
            client.create_buffer(first);
            client.create_buffer(second);
            SubsurfaceClient::commit(child, first);
            SubsurfaceClient::commit(child, second);
        }));

    ASSERT_THAT(first.buffer, NotNull());
    ASSERT_THAT(second.buffer, NotNull());
    EXPECT_TRUE(first.released);
}

// The buffer that replaced it is still waiting for the parent surface to be committed.
TEST_F(Subsurface, buffer_cached_while_synchronized_is_not_released)
{
    start_server();

    Buffer first;
    Buffer second;

    ASSERT_TRUE(run([&](SubsurfaceClient& client, wl_surface* child)
        {
            client.create_buffer(first);
            client.create_buffer(second);
            SubsurfaceClient::commit(child, first);
            SubsurfaceClient::commit(child, second);
        }));

    ASSERT_THAT(second.buffer, NotNull());
    EXPECT_FALSE(second.released);
}

// Committing the cached buffer again doesn't replace it.
TEST_F(Subsurface, buffer_committed_again_while_synchronized_is_not_released)
{
    start_server();

    Buffer buffer;

    ASSERT_TRUE(run([&](SubsurfaceClient& client, wl_surface* child)
        {
            client.create_buffer(buffer);
            SubsurfaceClient::commit(child, buffer);
            SubsurfaceClient::commit(child, buffer);
        }));

    ASSERT_THAT(buffer.buffer, NotNull());
    EXPECT_FALSE(buffer.released);
}

// "If a surface's parent surface behaves as desynchronized, then the cached state is applied
// on set_desync." A frame request committed without a buffer is answered once it has been
// applied, which makes that visible to the client.
TEST_F(Subsurface, cached_state_is_applied_on_set_desync)
{
    start_server();

    bool frame_done = false;

    ASSERT_TRUE(run(
        [&](SubsurfaceClient& client, wl_surface* child)
        {
            SubsurfaceClient::commit_frame_request(child, frame_done);
            client.set_desync();
        },
        [&] { return frame_done; }));

    EXPECT_TRUE(frame_done);
}

// Without set_desync the state stays cached, as the parent surface is never committed.
TEST_F(Subsurface, cached_state_is_not_applied_while_synchronized)
{
    start_server();

    bool frame_done = false;

    ASSERT_TRUE(run(
        [&](SubsurfaceClient&, wl_surface* child) { SubsurfaceClient::commit_frame_request(child, frame_done); },
        [&] { return frame_done; }));

    EXPECT_FALSE(frame_done);
}
