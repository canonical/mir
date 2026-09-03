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

#include "wayland_rs_server_test.h"

#include <mir/synchronised.h>

#include <mir_test_framework/temporary_environment_value.h>

#include <gtest/gtest.h>

#include <wayland-client.h>

#include <sys/socket.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace mrs = mir::wayland_rs;

namespace
{
class CountingNotificationHandler : public mrs::WaylandServerNotificationHandler
{
public:
    void client_added(rust::Box<mrs::WaylandClient>) override
    {
        ++*count.lock();
        cv.notify_all();
    }

    void client_removed(rust::Box<mrs::WaylandClientId>) override {}

    /// Wait until at least `n` clients have been adopted, or the timeout elapses.
    auto wait_for_clients(std::size_t n) -> bool
    {
        using namespace std::chrono_literals;
        auto constexpr timeout = 5s;
        auto locked = count.lock();
        return locked.wait_for(cv, timeout, [&] { return *locked >= n; });
    }

    auto client_count() -> std::size_t
    {
        return *count.lock();
    }

private:
    mir::Synchronised<std::size_t> count{0};
    std::condition_variable cv;
};

auto make_socket_pair() -> std::pair<int, int>
{
    int fds[2]{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        throw std::runtime_error{"Failed to create socket pair"};

    return {fds[0], fds[1]};
}

class ClientInjectionTest : public mrs::test::RunningWaylandServerTest
{
public:
    auto make_notification_handler() -> std::unique_ptr<mrs::WaylandServerNotificationHandler> override
    {
        auto handler_owned = std::make_unique<CountingNotificationHandler>();
        handler = handler_owned.get();
        return handler_owned;
    }

    void TearDown() override
    {
        for (auto* display : displays)
            wl_display_disconnect(display);

        RunningWaylandServerTest::TearDown();
    }

    /// Adopt the server end of a fresh socket pair via `insert_client` and drive
    /// the client end with a `wl_display`. Returns the connected display, which
    /// this fixture disconnects in `TearDown`.
    auto inject_client() -> wl_display*
    {
        auto const [server_fd, client_fd] = make_socket_pair();

        // Ownership of the client end transfers to the wl_display, which closes
        // it on disconnect (and on failure here). This is the only step that can
        // fail, so on that early exit we close the server end we have not yet
        // handed off; on success it transfers to the server via insert_client.
        auto* display = wl_display_connect_to_fd(client_fd);
        if (!display)
        {
            ::close(server_fd);
            throw std::runtime_error{"Failed to connect Wayland client to injected fd"};
        }

        (*server)->insert_client(server_fd);

        displays.push_back(display);
        return display;
    }

    CountingNotificationHandler* handler{nullptr};
    std::vector<wl_display*> displays;
};
}

TEST_F(ClientInjectionTest, injected_socket_fd_is_adopted_as_a_client)
{
    auto* const display = inject_client();
    wl_display_roundtrip(display);

    EXPECT_TRUE(handler->wait_for_clients(1)) << "The server did not adopt the injected socket as a client";
}

TEST_F(ClientInjectionTest, injected_client_can_complete_a_roundtrip)
{
    auto* const display = inject_client();

    // A successful roundtrip proves the adopted socket is a fully-functional
    // client connection the server dispatches, not merely a registered fd.
    EXPECT_NE(wl_display_roundtrip(display), -1) << "Roundtrip over the injected client connection failed";
    EXPECT_EQ(wl_display_get_error(display), 0) << "The injected client connection reported a protocol error";
}

TEST_F(ClientInjectionTest, multiple_injected_clients_are_all_adopted)
{
    std::size_t const client_count = 3;
    for (std::size_t i = 0; i < client_count; ++i)
        wl_display_roundtrip(inject_client());

    EXPECT_TRUE(handler->wait_for_clients(client_count)) << "Not every injected socket was adopted as a client";
    EXPECT_EQ(handler->client_count(), client_count);
}

TEST(ClientInjectionBeforeRunTest, client_injected_before_run_is_adopted)
{
    auto const runtime_dir = mrs::test::make_temp_runtime_dir();
    mir_test_framework::TemporaryEnvironmentValue const xdg_runtime_dir{"XDG_RUNTIME_DIR", runtime_dir.c_str()};

    auto server = mrs::create_wayland_server();
    auto handler_owned = std::make_unique<CountingNotificationHandler>();
    auto* const handler = handler_owned.get();
    auto executor = std::make_unique<mrs::WaylandExecutor>(*server);

    auto const [server_fd, client_fd] = make_socket_pair();

    // Inject before the server is running: the client signal does not yet exist,
    // so this must be queued and drained when `run` begins.
    server->insert_client(server_fd);

    std::string const socket = "mir-wayland-rs-test-prerun-" + std::to_string(::getpid());
    std::thread server_thread{[&, notification_handler = std::move(handler_owned), work = std::move(executor)]() mutable
                              { server->run(socket, nullptr, std::move(notification_handler), std::move(work)); }};

    auto* display = wl_display_connect_to_fd(client_fd);
    ASSERT_TRUE(display) << "Failed to connect Wayland client to injected fd";
    wl_display_roundtrip(display);

    EXPECT_TRUE(handler->wait_for_clients(1)) << "A client injected before the server started was not adopted";

    wl_display_disconnect(display);
    server->stop();
    server_thread.join();

    std::error_code ec;
    std::filesystem::remove_all(runtime_dir, ec);
}
