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

void trivial_client(wl_display* display)
{
    std::unique_ptr<wl_registry, decltype(&wl_registry_destroy)> registry{wl_display_get_registry(display), &wl_registry_destroy};
    wl_display_roundtrip(display);
}
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