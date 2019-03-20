/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "test_server.h"

#include <miral/internal_client.h>
#include <miral/wayland_extensions.h>

#include <wayland-client.h>

#include <memory>
#include <mutex>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


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
        std::lock_guard<decltype(mutex)> lock{mutex};
        session_ = session;
    }

    std::shared_ptr<mir::scene::Session> session() const
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return session_.lock();
    }

    std::function<void (struct wl_display*)> code = [](auto){};

private:
    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;
};

struct WaylandExtensions : miral::TestServer
{
    void SetUp() override
    {
        testing::Test::SetUp();
        add_server_init(launcher);
    }

    void add_server_init(std::function<void(mir::Server&)>&& init)
    {
        auto temp = [old_init=init_server, new_init=init](mir::Server& server)
            {
                old_init(server);
                new_init(server);
            };

        init_server = temp;
    }

    void run_as_client(std::function<void (struct wl_display*)>&& code)
    {
        bool client_run = false;
        std::condition_variable cv;
        std::mutex mutex;

        client.code = [&](struct wl_display* display)
            {
                std::lock_guard<decltype(mutex)> lock{mutex};
                code(display);
                client_run = true;
                cv.notify_one();
            };

        std::unique_lock<decltype(mutex)> lock{mutex};
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

void trivial_client(wl_display* display)
{
    auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);
    wl_display_roundtrip(display);
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
        InterfaceNames* const interfaces = static_cast<decltype(interfaces)>(data);
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
}

TEST_F(WaylandExtensions, client_connects)
{
    bool client_connected = false;
    start_server();

    run_as_client([&](auto) { client_connected = true; });

    EXPECT_THAT(client_connected, Eq(true));
}

TEST_F(WaylandExtensions, filter_is_called)
{
    bool filter_called = false;

    miral::WaylandExtensions extensions;
    add_server_init(extensions);
    extensions.set_filter([&](auto, auto) { filter_called = true; return true; });
    start_server();

    run_as_client(trivial_client);

    EXPECT_THAT(filter_called, Eq(true));
}

TEST_F(WaylandExtensions, client_sees_default_extensions)
{
    ClientGlobalEnumerator enumerator_client;
    miral::WaylandExtensions extensions;
    add_server_init(extensions);
    start_server();

    run_as_client(enumerator_client);

    auto const available_extensions = extensions.supported_extensions();
    available_extensions += ':';

    for (char const* start = available_extensions.c_str(); char const* end = strchr(start, ':'); start = end+1)
    {
        if (start != end)
        {
            EXPECT_THAT(*enumerator_client.interfaces, Contains(Eq(std::string{start, end})));
        }
    }
}