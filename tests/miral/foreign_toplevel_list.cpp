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
#include <miral/wayland_extensions.h>
#include <miral/window_manager_tools.h>
#include <miral/window_specification.h>

#include <wayland-client.h>

#include <sys/mman.h>
#include <unistd.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>

using namespace testing;

namespace mir::wayland
{
extern wl_interface const xdg_wm_base_interface_data;
extern wl_interface const xdg_surface_interface_data;
extern wl_interface const xdg_toplevel_interface_data;
extern wl_interface const ext_foreign_toplevel_list_v1_interface_data;
}

namespace
{
void destroy_proxy(wl_proxy* proxy)
{
    wl_proxy_marshal_flags(proxy, 0, nullptr, wl_proxy_get_version(proxy), WL_MARSHAL_FLAG_DESTROY);
}

using Proxy = std::unique_ptr<wl_proxy, decltype(&destroy_proxy)>;

struct Handle
{
    explicit Handle(wl_proxy* proxy) : proxy{proxy, destroy_proxy}
    {
        wl_proxy_add_listener(proxy, listener, this);
    }

    static void closed(void* data, wl_proxy*)
    {
        ++static_cast<Handle*>(data)->closed_count;
    }

    static void done(void* data, wl_proxy*)
    {
        ++static_cast<Handle*>(data)->updates;
    }

    static void property(void* data, wl_proxy*, char const*)
    {
        ++static_cast<Handle*>(data)->updates;
    }

    static void identifier(void* data, wl_proxy*, char const* identifier)
    {
        auto& self = *static_cast<Handle*>(data);
        self.identifier_ = identifier;
        ++self.updates;
    }

    static inline void (*listener[])(){
        reinterpret_cast<void (*)()>(closed),
        reinterpret_cast<void (*)()>(done),
        reinterpret_cast<void (*)()>(property),
        reinterpret_cast<void (*)()>(property),
        reinterpret_cast<void (*)()>(identifier)};

    Proxy proxy;
    std::string identifier_;
    int closed_count{0};
    int updates{0};
};

struct Client
{
    explicit Client(wl_display* display) : display{display}
    {
        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        roundtrip();
    }

    ~Client()
    {
        stop_list();
        handles.clear();
        toplevel.reset();
        xdg_surface.reset();
        if (surface) wl_surface_destroy(surface);
        if (buffer) wl_buffer_destroy(buffer);
        wm_base.reset();
        if (shm) wl_shm_destroy(shm);
        if (compositor) wl_compositor_destroy(compositor);
        wl_registry_destroy(registry);
    }

    void roundtrip()
    {
        // Scene notifications are dispatched through the Wayland executor.
        EXPECT_THAT(wl_display_roundtrip(display), Ge(0));
        EXPECT_THAT(wl_display_roundtrip(display), Ge(0));
    }

    void bind_list()
    {
        ASSERT_THAT(list_id, Ne(0u));
        list = static_cast<wl_proxy*>(wl_registry_bind(
            registry, list_id, &mir::wayland::ext_foreign_toplevel_list_v1_interface_data, 1));
        wl_proxy_add_listener(list, list_listener, this);
        roundtrip();
    }

    void stop_list()
    {
        if (!list) return;
        wl_proxy_marshal(list, 0); // stop
        roundtrip();
        wl_proxy_marshal_flags(list, 1, nullptr, 1, WL_MARSHAL_FLAG_DESTROY);
        list = nullptr;
        roundtrip();
    }

    void create_toplevel()
    {
        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(shm, NotNull());
        ASSERT_THAT(wm_base, NotNull());
        surface = wl_compositor_create_surface(compositor);
        xdg_surface.reset(wl_proxy_marshal_constructor(
            wm_base.get(), 2, &mir::wayland::xdg_surface_interface_data, nullptr, surface));
        wl_proxy_add_listener(xdg_surface.get(), surface_listener, this);
        toplevel.reset(wl_proxy_marshal_constructor(
            xdg_surface.get(), 1, &mir::wayland::xdg_toplevel_interface_data));
        wl_surface_commit(surface);
        roundtrip();
        ASSERT_TRUE(configured);

        int constexpr stride = 100 * 4;
        int constexpr size = stride * 100;
        auto const fd = memfd_create("foreign-toplevel-test", MFD_CLOEXEC);
        ASSERT_THAT(fd, Ge(0));
        auto const result = ftruncate(fd, size);
        if (result != 0) close(fd);
        ASSERT_THAT(result, Eq(0));
        auto* const pool = wl_shm_create_pool(shm, fd, size);
        close(fd);
        buffer = wl_shm_pool_create_buffer(pool, 0, 100, 100, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        map();
    }

    void map()
    {
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_commit(surface);
        roundtrip();
    }

    void unmap()
    {
        wl_surface_attach(surface, nullptr, 0, 0);
        wl_surface_commit(surface);
        roundtrip();
    }

    void remap()
    {
        configured = false;
        wl_surface_commit(surface);
        roundtrip();
        ASSERT_TRUE(configured);
        map();
    }

    static void global(void* data, wl_registry* registry, uint32_t id, char const* name, uint32_t)
    {
        auto& self = *static_cast<Client*>(data);
        auto bind = [&](wl_interface const& interface, auto& out)
        {
            if (std::string_view{name} == interface.name)
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    wl_registry_bind(registry, id, &interface, 1));
        };
        bind(wl_compositor_interface, self.compositor);
        bind(wl_shm_interface, self.shm);
        if (std::string_view{name} == "xdg_wm_base")
            self.wm_base.reset(static_cast<wl_proxy*>(
                wl_registry_bind(registry, id, &mir::wayland::xdg_wm_base_interface_data, 1)));
        if (std::string_view{name} == "ext_foreign_toplevel_list_v1")
            self.list_id = id;
    }

    static void global_remove(void*, wl_registry*, uint32_t) {}

    static void configure(void* data, wl_proxy* surface, uint32_t serial)
    {
        wl_proxy_marshal(surface, 4, serial); // ack_configure
        static_cast<Client*>(data)->configured = true;
    }

    static void toplevel_created(void* data, wl_proxy*, wl_proxy* handle)
    {
        static_cast<Client*>(data)->handles.push_back(std::make_unique<Handle>(handle));
    }

    static void finished(void*, wl_proxy*) {}

    static constexpr wl_registry_listener registry_listener{global, global_remove};
    static inline void (*surface_listener[])(){reinterpret_cast<void (*)()>(configure)};
    static inline void (*list_listener[])(){
        reinterpret_cast<void (*)()>(toplevel_created),
        reinterpret_cast<void (*)()>(finished)};

    wl_display* const display;
    wl_registry* registry{nullptr};
    wl_compositor* compositor{nullptr};
    wl_shm* shm{nullptr};
    Proxy wm_base{nullptr, destroy_proxy};
    Proxy xdg_surface{nullptr, destroy_proxy};
    Proxy toplevel{nullptr, destroy_proxy};
    wl_surface* surface{nullptr};
    wl_buffer* buffer{nullptr};
    wl_proxy* list{nullptr};
    uint32_t list_id{0};
    bool configured{false};
    std::vector<std::unique_ptr<Handle>> handles;
};

struct InternalClient
{
    void operator()(wl_display* display) { code(display); }
    void operator()(std::weak_ptr<mir::scene::Session> const&) {}
    std::function<void(wl_display*)> code;
};

struct ForeignToplevelList : miral::TestServer
{
    ForeignToplevelList()
    {
        add_server_init(launcher);
        miral::WaylandExtensions extensions;
        extensions.enable("ext_foreign_toplevel_list_v1");
        add_server_init(extensions);
    }

    void run_as_client(std::function<void(Client&)>&& code)
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done{false};
        internal_client.code = [&](wl_display* display)
        {
            {
                Client client{display};
                client.bind_list();
                client.create_toplevel();
                code(client);
            }
            {
                std::lock_guard lock{mutex};
                done = true;
            }
            cv.notify_one();
        };
        launcher.launch(internal_client);
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return done; });
    }

    void set_window_state(MirWindowState state)
    {
        invoke_tools([&](miral::WindowManagerTools& tools)
        {
            auto const window = tools.active_window();
            ASSERT_TRUE(window);
            miral::WindowSpecification mods;
            mods.state() = state;
            tools.modify_window(window, mods);
        });
    }

    miral::InternalClientLauncher launcher;
    InternalClient internal_client;
};
}

TEST_F(ForeignToplevelList, null_buffer_closes_handle_and_remap_creates_new_identifier)
{
    run_as_client([](Client& client)
    {
        ASSERT_THAT(client.handles, SizeIs(1));
        auto& old_handle = *client.handles.front();
        auto const identifier = old_handle.identifier_;
        ASSERT_FALSE(identifier.empty());

        client.unmap();
        EXPECT_THAT(old_handle.closed_count, Eq(1));
        auto const updates = old_handle.updates;
        wl_proxy_marshal(client.toplevel.get(), 2, "unmapped title");
        client.roundtrip();
        EXPECT_THAT(old_handle.updates, Eq(updates));

        client.remap();
        ASSERT_THAT(client.handles, SizeIs(2));
        EXPECT_FALSE(client.handles.back()->identifier_.empty());
        EXPECT_THAT(client.handles.back()->identifier_, Ne(identifier));
        EXPECT_THAT(old_handle.closed_count, Eq(1));
        EXPECT_THAT(old_handle.updates, Eq(updates));

        // The old handle is still client-owned: destroying it must not cause a protocol error.
        old_handle.proxy.reset();
        client.roundtrip();
    });
}

TEST_F(ForeignToplevelList, metadata_only_commit_does_not_remap_null_buffer)
{
    run_as_client([](Client& client)
    {
        ASSERT_THAT(client.handles, SizeIs(1));
        client.unmap();
        wl_surface_set_buffer_scale(client.surface, 2);
        wl_surface_commit(client.surface);
        client.roundtrip();
        EXPECT_THAT(client.handles, SizeIs(1));
        EXPECT_THAT(client.handles.front()->closed_count, Eq(1));
    });
}

TEST_F(ForeignToplevelList, remap_without_a_bound_list_changes_identifier)
{
    run_as_client([](Client& client)
    {
        ASSERT_THAT(client.handles, SizeIs(1));
        auto const identifier = client.handles.front()->identifier_;
        client.stop_list();
        client.unmap();
        client.remap();
        client.bind_list();
        ASSERT_THAT(client.handles, SizeIs(2));
        EXPECT_THAT(client.handles.back()->identifier_, Ne(identifier));
    });
}

TEST_F(ForeignToplevelList, list_bindings_share_identifiers_across_unmap_and_remap)
{
    run_as_client([](Client& client)
    {
        Client other{client.display};
        other.bind_list();
        ASSERT_THAT(client.handles, SizeIs(1));
        ASSERT_THAT(other.handles, SizeIs(1));
        auto const identifier = client.handles.front()->identifier_;
        EXPECT_THAT(other.handles.front()->identifier_, Eq(identifier));

        client.unmap();
        client.remap();

        EXPECT_THAT(client.handles.front()->closed_count, Eq(1));
        EXPECT_THAT(other.handles.front()->closed_count, Eq(1));
        ASSERT_THAT(client.handles, SizeIs(2));
        ASSERT_THAT(other.handles, SizeIs(2));
        EXPECT_THAT(client.handles.back()->identifier_, Ne(identifier));
        EXPECT_THAT(other.handles.back()->identifier_, Eq(client.handles.back()->identifier_));
    });
}

TEST_F(ForeignToplevelList, compositor_hidden_window_keeps_handle_until_client_unmaps)
{
    run_as_client([&](Client& client)
    {
        ASSERT_THAT(client.handles, SizeIs(1));
        set_window_state(mir_window_state_hidden);
        client.roundtrip();
        EXPECT_THAT(client.handles.front()->closed_count, Eq(0));
        client.unmap();
        EXPECT_THAT(client.handles.front()->closed_count, Eq(1));
    });
}

TEST_F(ForeignToplevelList, minimized_window_keeps_handle_until_client_unmaps)
{
    run_as_client([&](Client& client)
    {
        ASSERT_THAT(client.handles, SizeIs(1));
        set_window_state(mir_window_state_minimized);
        client.roundtrip();
        EXPECT_THAT(client.handles.front()->closed_count, Eq(0));
        client.unmap();
        EXPECT_THAT(client.handles.front()->closed_count, Eq(1));
    });
}
