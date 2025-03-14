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
#include "server_example_decoration.h"
#include "org_kde_kwin_server_decoration.h"

#include <miral/internal_client.h>
#include <miral/wayland_extensions.h>

#include <wayland-client.h>

#include <memory>
#include <mutex>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/abnormal_exit.h>

using namespace testing;

namespace
{
class WaylandClient
{
public:

    void operator()(struct wl_display* display)
    {
        code(display);
    }

    void operator()(std::weak_ptr<mir::scene::Session> const& session)
    {
        std::lock_guard lock{mutex};
        session_ = session;
    }

    std::shared_ptr<mir::scene::Session> session() const
    {
        std::lock_guard lock{mutex};
        return session_.lock();
    }

    std::function<void (struct wl_display*)> code = [](auto){};

private:
    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;
};

struct WaylandExtensions : miral::TestServer
{
    WaylandExtensions()
    {
        start_server_in_setup = false;
        add_server_init(launcher);
    }

    void run_as_client(std::function<void (struct wl_display*)>&& code)
    {
        bool client_run = false;
        std::condition_variable cv;
        std::mutex mutex;

        client.code = [&](struct wl_display* display)
            {
                {
                    std::lock_guard lock{mutex};
                    code(display);
                    client_run = true;
                }
                cv.notify_one();
            };

        std::unique_lock lock{mutex};
        launcher.launch(client);
        cv.wait(lock, [&]{ return client_run; });
    }

private:
    miral::InternalClientLauncher launcher;
    WaylandClient client;
};

template<typename Type>
auto make_scoped(Type* owned, void(*deleter)(Type*)) -> std::unique_ptr<Type, void(*)(Type*)>
{
    return {owned, deleter};
}

struct ClientGlobalEnumerator
{
    using InterfaceNames = std::vector<std::string>;

    void operator()(wl_display* display)
    {
        auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
        wl_registry_add_listener(registry.get(), &registry_listener, interfaces.get());
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);
    }

    static void new_global(
        void* data,
        struct wl_registry* /*registry*/,
        uint32_t /*id*/,
        char const* interface,
        uint32_t /*version*/)
    {
        auto const interfaces = static_cast<InterfaceNames*>(data);
        interfaces->push_back(interface);
    }

    static void global_remove(
        void* /*data*/,
        struct wl_registry* /*registry*/,
        uint32_t /*name*/)
    {

    }

    static wl_registry_listener constexpr registry_listener = {
        new_global,
        global_remove
    };

    std::shared_ptr<InterfaceNames> interfaces = std::make_shared<InterfaceNames>();
};

wl_registry_listener constexpr ClientGlobalEnumerator::registry_listener;

struct ClientDecorationCreator
{
    ClientDecorationCreator(std::function<void()>&& test) :
        test{test} {}

    std::function<void()> test = []{};

    void operator()(wl_display* display)
    {
        auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
        wl_registry_add_listener(registry.get(), &registry_listener, this);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        ASSERT_THAT(compositor, NotNull());
        ASSERT_THAT(decoration_manager, NotNull());
        ASSERT_THAT(shell, NotNull());

        auto const surface = make_scoped(
            wl_compositor_create_surface(compositor),
            &wl_surface_destroy);
        wl_display_roundtrip(display);

        auto const decoration = make_scoped(
            org_kde_kwin_server_decoration_manager_create(decoration_manager, surface.get()),
            &org_kde_kwin_server_decoration_release);
        wl_display_roundtrip(display);

        auto const window = make_scoped(wl_shell_get_shell_surface(shell, surface.get()), &wl_shell_surface_destroy);
        wl_shell_surface_set_toplevel(window.get());
        wl_display_roundtrip(display);

        wl_surface_commit(surface.get());
        wl_display_roundtrip(display);

        test();
    }

    static void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t /*version*/)
    {
        ClientDecorationCreator* self = static_cast<decltype(self)>(data);

        if (strcmp(interface, wl_compositor_interface.name) == 0)
        {
            self->compositor = static_cast<decltype(self->compositor)>
            (wl_registry_bind(registry, id, &wl_compositor_interface, 1));
        }

        if (strcmp(interface, org_kde_kwin_server_decoration_manager_interface.name) == 0)
        {
            self->decoration_manager = static_cast<decltype(self->decoration_manager)>
                (wl_registry_bind(registry, id, &org_kde_kwin_server_decoration_manager_interface, 1));
        }

        if (strcmp(interface, wl_shell_interface.name) == 0)
        {
            self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
        }
    }

    static void global_remove(
        void* /*data*/,
        struct wl_registry* /*registry*/,
        uint32_t /*name*/)
    {

    }

    static wl_registry_listener constexpr registry_listener = {
        new_global,
        global_remove
    };

    wl_compositor* compositor = nullptr;
    org_kde_kwin_server_decoration_manager* decoration_manager = nullptr;
    wl_shell* shell = nullptr;
};

wl_registry_listener constexpr ClientDecorationCreator::registry_listener;
}

TEST_F(WaylandExtensions, client_connects)
{
    bool client_connected = false;
    start_server();

    run_as_client([&](auto) { client_connected = true; });

    EXPECT_THAT(client_connected, Eq(true));
}

TEST_F(WaylandExtensions, client_sees_default_extensions)
{
    ClientGlobalEnumerator enumerator_client;
    start_server();

    run_as_client(enumerator_client);

    auto const available_extensions = miral::WaylandExtensions::recommended();

    // Some extension names do not include the *_manager like their globals's name does.
    // Fix by adding a non-manager version to the available list for each interface that contains "_manager".
    std::vector<std::string> available = *enumerator_client.interfaces;
    std::string const to_remove = "_manager";
    for (unsigned i = 0; i < available.size(); i++)
    {
        auto const pos = available[i].find(to_remove);
        if (pos != std::string::npos)
        {
            auto str = available[i];
            str.erase(pos, to_remove.size());
            available.push_back(str);
        }
    }

    for (auto const& extension : available_extensions)
    {
        EXPECT_THAT(available, Contains(Eq(extension)));
    }
}

TEST_F(WaylandExtensions, add_extension_adds_protocol_to_supported_enabled_extensions)
{
    miral::WaylandExtensions extensions;

    EXPECT_THAT(extensions.all_supported(), Not(Contains(Eq(mir::examples::server_decoration_extension().name))));

    extensions.add_extension(mir::examples::server_decoration_extension());

    EXPECT_THAT(extensions.all_supported(), Contains(Eq(mir::examples::server_decoration_extension().name)));
}

TEST_F(WaylandExtensions, add_extension_disabled_by_default_adds_protocol_to_supported_extensions_only)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension_disabled_by_default(mir::examples::server_decoration_extension());
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(mir::examples::server_decoration_extension().name))));
}

TEST_F(WaylandExtensions, can_retrieve_application_for_client)
{
    miral::WaylandExtensions extensions;
    wl_client* client = nullptr;

    extensions.add_extension(mir::examples::server_decoration_extension([&](wl_client* c, auto){ client = c; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(client, NotNull());
            EXPECT_THAT(miral::application_for(client), NotNull());
        }});
}

TEST_F(WaylandExtensions, can_retrieve_application_for_surface)
{
    miral::WaylandExtensions extensions;

    wl_resource* surface = nullptr;
    extensions.add_extension(mir::examples::server_decoration_extension([&](auto, wl_resource* s){ surface =s; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(surface, NotNull());
            EXPECT_THAT(miral::application_for(surface), NotNull());
        }});
}


TEST_F(WaylandExtensions, can_retrieve_window_for_surface)
{
    miral::WaylandExtensions extensions;

    wl_resource* surface = nullptr;
    extensions.add_extension(mir::examples::server_decoration_extension([&](auto, wl_resource* s){ surface =s; }));

    add_server_init(extensions);
    start_server();

    run_as_client(ClientDecorationCreator{[&]
        {
            EXPECT_THAT(surface, NotNull());
            EXPECT_THAT(miral::window_for(surface), Ne(nullptr));   // NotNull() fails to build on 16.04LTS
        }});
}

TEST_F(WaylandExtensions, some_extensions_are_supported)
{
    EXPECT_THAT(miral::WaylandExtensions::supported(), Not(IsEmpty()));
}

TEST_F(WaylandExtensions, some_extensions_are_recommended)
{
    EXPECT_THAT(miral::WaylandExtensions::recommended(), Not(IsEmpty()));
}

TEST_F(WaylandExtensions, recommeded_extensions_subset_of_supported_extensions)
{
    for (auto const& e : miral::WaylandExtensions::recommended())
    {
        EXPECT_THAT(miral::WaylandExtensions::supported(), Contains(e));
    }
}

TEST_F(WaylandExtensions, disable_can_remove_default_extensions)
{
    std::string const extension_to_remove{"zxdg_shell_v6"};
    miral::WaylandExtensions extensions;
    extensions.disable(extension_to_remove);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(extension_to_remove))));
}

TEST_F(WaylandExtensions, enable_can_enable_non_standard_extensions)
{
    std::string const extension_to_enable{"zwlr_layer_shell_v1"};
    miral::WaylandExtensions extensions;
    extensions.enable(extension_to_enable);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(extension_to_enable)));
}

TEST_F(WaylandExtensions, enable_can_enable_bespoke_extension)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension_disabled_by_default(mir::examples::server_decoration_extension());
    extensions.enable(mir::examples::server_decoration_extension().name);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(mir::examples::server_decoration_extension().name)));
}

TEST_F(WaylandExtensions, disable_can_disable_bespoke_extension)
{
    miral::WaylandExtensions extensions;
    extensions.add_extension(mir::examples::server_decoration_extension());
    extensions.disable(mir::examples::server_decoration_extension().name);
    ClientGlobalEnumerator enumerator_client;

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq(mir::examples::server_decoration_extension().name))));
}

TEST_F(WaylandExtensions, cannot_duplicate_existing_extension)
{
    miral::WaylandExtensions::Builder duplicate_extension{
        "zwlr_layer_shell_v1",
        [&](auto const&){ return std::shared_ptr<void>{};}};

    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    EXPECT_THROW(
        extensions.add_extension(duplicate_extension),
        mir::AbnormalExit);

    EXPECT_THROW(
        extensions.add_extension_disabled_by_default(duplicate_extension),
        mir::AbnormalExit);
}

TEST_F(WaylandExtensions, cannot_duplicate_bespoke_extension)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    extensions.add_extension(mir::examples::server_decoration_extension());

    EXPECT_THROW(
        extensions.add_extension(mir::examples::server_decoration_extension()),
        mir::AbnormalExit);

    EXPECT_THROW(
        extensions.add_extension_disabled_by_default(mir::examples::server_decoration_extension()),
        mir::AbnormalExit);
}

TEST_F(WaylandExtensions, wayland_extensions_option_sets_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_WAYLAND_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("xdg_wm_base")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("wl_shell"))));
}

TEST_F(WaylandExtensions, add_extensions_option_adds_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_ADD_WAYLAND_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("wl_shell")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("xdg_wm_base")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
}

TEST_F(WaylandExtensions, drop_extensions_option_removes_extensions)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_DROP_WAYLAND_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("wl_shell")));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("xdg_wm_base"))));
    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("zwlr_layer_shell_v1"))));
}

TEST_F(WaylandExtensions, add_extensions_option_adds_extensions_if_name_is_not_global_name)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_ADD_WAYLAND_EXTENSIONS", "zwp_input_method_v2:zwp_virtual_keyboard_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwp_input_method_manager_v2")));
    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwp_virtual_keyboard_manager_v1")));
}

TEST_F(WaylandExtensions, drop_extensions_option_overrides_enable)
{
    miral::WaylandExtensions extensions;
    extensions.enable("zwlr_layer_shell_v1");
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_DROP_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("zwlr_layer_shell_v1"))));
}

TEST_F(WaylandExtensions, add_extensions_option_overrides_disable)
{
    miral::WaylandExtensions extensions;
    extensions.disable("zwlr_layer_shell_v1");
    ClientGlobalEnumerator enumerator_client;

    add_to_environment("MIR_SERVER_ADD_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
}

TEST_F(WaylandExtensions, conditionally_enable_given_correct_extension_name)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.name(), StrEq("zwlr_layer_shell_v1"));
            condition_called = true;
            return true;
        });

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_not_given_user_preference_if_none_specified)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.user_preference(), Eq(std::nullopt));
            condition_called = true;
            return true;
        });

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_given_user_preference_true_if_added)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.user_preference(), Eq(std::make_optional(true)));
            condition_called = true;
            return true;
        });
    add_to_environment("MIR_SERVER_ADD_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_given_user_preference_false_if_dropped)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.user_preference(), Eq(std::make_optional(false)));
            condition_called = true;
            return true;
        });
    add_to_environment("MIR_SERVER_DROP_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_given_user_preference_true_if_in_extensions_list)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.user_preference(), Eq(std::make_optional(true)));
            condition_called = true;
            return true;
        });
    add_to_environment("MIR_SERVER_WAYLAND_EXTENSIONS", "xdg_wm_base:zwlr_layer_shell_v1");

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_given_user_preference_false_if_not_in_extensions_list)
{
    miral::WaylandExtensions extensions;
    bool condition_called = false;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const& info)
        {
            EXPECT_THAT(info.user_preference(), Eq(std::make_optional(false)));
            condition_called = true;
            return true;
        });
    add_to_environment("MIR_SERVER_WAYLAND_EXTENSIONS", "xdg_wm_base");

    add_server_init(extensions);
    start_server();
    run_as_client(ClientGlobalEnumerator{});

    EXPECT_THAT(condition_called, Eq(true));
}

TEST_F(WaylandExtensions, conditionally_enable_can_enable_extension)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const&)
        {
            return true;
        });

    add_server_init(extensions);
    start_server();
    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
}

TEST_F(WaylandExtensions, conditionally_enable_can_disable_extension)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const&)
        {
            return false;
        });

    add_server_init(extensions);
    start_server();
    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("zwlr_layer_shell_v1"))));
}

TEST_F(WaylandExtensions, conditionally_enable_can_enable_extension_disabled_by_user)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const&)
        {
            return true;
        });

    add_to_environment("MIR_SERVER_DROP_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");
    add_server_init(extensions);
    start_server();
    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq("zwlr_layer_shell_v1")));
}

TEST_F(WaylandExtensions, conditionally_enable_can_disable_extension_enabled_by_user)
{
    miral::WaylandExtensions extensions;
    ClientGlobalEnumerator enumerator_client;
    extensions.conditionally_enable("zwlr_layer_shell_v1", [&](auto const&)
        {
            return false;
        });

    add_to_environment("MIR_SERVER_ADD_WAYLAND_EXTENSIONS", "zwlr_layer_shell_v1");
    add_server_init(extensions);
    start_server();
    run_as_client(enumerator_client);

    EXPECT_THAT(*enumerator_client.interfaces, Not(Contains(Eq("zwlr_layer_shell_v1"))));
}
