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

#include "src/server/frontend_wayland/wayland_client_notifier.h"

#include "tests/wayland_rs/wayland_rs_server_test.h"

#include "client.h"
#include "wayland_client_registry.h"

#include <mir/executor.h>
#include <mir/fd.h>
#include <mir/frontend/session_authorizer.h>
#include <mir/frontend/session_credentials.h>
#include <mir/scene/session.h>
#include <mir/shell/shell.h>
#include <mir/test/doubles/stub_session.h>
#include <mir/test/doubles/stub_shell.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <wayland-client.h>

#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace mf = mir::frontend;
namespace mrs = mir::wayland_rs;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
auto constexpr timeout = 5s;

struct MockShell : mtd::StubShell
{
    MOCK_METHOD((std::shared_ptr<ms::Session>), open_session, (pid_t, mir::Fd, std::string const&), (override));
    MOCK_METHOD(void, close_session, (std::shared_ptr<ms::Session> const&), (override));
};

struct MockSessionAuthorizer : mf::SessionAuthorizer
{
    MOCK_METHOD(bool, connection_is_allowed, (mf::SessionCredentials const&), (override));
};

auto make_socket_pair() -> std::pair<int, int>
{
    int fds[2]{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        throw std::runtime_error{"Failed to create socket pair"};

    return {fds[0], fds[1]};
}

/// Block until `future` is ready (or the timeout elapses).
template<typename T>
auto is_ready(std::future<T>& future) -> bool
{
    return future.wait_for(timeout) == std::future_status::ready;
}

class WaylandClientNotifierTest : public mrs::test::RunningWaylandServerTest
{
public:
    struct ConnectionInfo
    {
        int socket_fd;
        std::shared_ptr<ms::Session> session;
        std::shared_ptr<mrs::Client> client;
    };

    void SetUp() override
    {
        // Happy-path defaults so tests that don't care about authorization or
        // session creation get a working, allowed client. Tests override these
        // with `EXPECT_CALL` where they need to observe or change the behaviour.
        ON_CALL(*shell, open_session(_, _, _))
            .WillByDefault(Invoke([](pid_t pid, mir::Fd, std::string const&)
                                  { return std::make_shared<mtd::StubSession>(pid); }));
        ON_CALL(*authorizer, connection_is_allowed(_)).WillByDefault(Return(true));

        RunningWaylandServerTest::SetUp();
    }

    void TearDown() override
    {
        for (auto* display : displays)
            wl_display_disconnect(display);

        RunningWaylandServerTest::TearDown();
    }

    auto make_notification_handler() -> std::unique_ptr<mrs::WaylandServerNotificationHandler> override
    {
        return std::make_unique<mf::WaylandClientNotifier>(
            shell,
            authorizer,
            registry,
            serial_source,
            [this](
                int socket_fd, std::shared_ptr<ms::Session> const& session, std::shared_ptr<mrs::Client> const& client)
            {
                if (on_connected)
                    on_connected(socket_fd, session, client);
            });
    }

    /// Arrange to capture the next connection the notifier reports, before
    /// injecting the client that produces it.
    auto next_connection() -> std::future<ConnectionInfo>
    {
        auto promise = std::make_shared<std::promise<ConnectionInfo>>();
        on_connected =
            [promise](
                int socket_fd, std::shared_ptr<ms::Session> const& session, std::shared_ptr<mrs::Client> const& client)
        { promise->set_value({socket_fd, session, client}); };
        return promise->get_future();
    }

    auto inject_client() -> std::pair<wl_display*, int>
    {
        auto const [server_fd, client_fd] = make_socket_pair();

        auto* display = wl_display_connect_to_fd(client_fd);
        if (!display)
        {
            ::close(server_fd);
            ::close(client_fd);
            throw std::runtime_error{"Failed to connect Wayland client to injected fd"};
        }

        (*server)->insert_client(server_fd);

        // Drive the connection so the server adopts and dispatches the client.
        wl_display_roundtrip(display);

        displays.push_back(display);
        return {display, server_fd};
    }

    /// Run `work` on the server's event-loop thread and return its result. Used
    /// to query the (unsynchronised) registry without racing the server, and as
    /// a barrier that flushes any pending client processing.
    template<typename Work>
    auto on_server_thread(Work&& work) -> std::invoke_result_t<Work&>
    {
        std::promise<std::invoke_result_t<Work&>> promise;
        auto future = promise.get_future();
        executor->spawn([&] { promise.set_value(work()); });
        return future.get();
    }

    /// Poll the client connection until the server disconnects it (as happens
    /// when the notifier kills a rejected client). Reads pending events without
    /// issuing new requests so a queued `wl_display.error` is observed rather
    /// than masked by a write to the already-closed socket.
    auto wait_for_server_disconnect(wl_display* display) -> bool
    {
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            wl_display_flush(display);
            if (wl_display_dispatch(display) < 0)
                return wl_display_get_error(display) != 0;
            std::this_thread::sleep_for(20ms);
        }
        return false;
    }

    std::shared_ptr<NiceMock<MockShell>> const shell = std::make_shared<NiceMock<MockShell>>();
    std::shared_ptr<NiceMock<MockSessionAuthorizer>> const authorizer =
        std::make_shared<NiceMock<MockSessionAuthorizer>>();
    mrs::WaylandClientRegistry registry;
    mf::WaylandSerialSource const serial_source = std::make_shared<std::atomic<uint32_t>>(0);

    std::function<void(int, std::shared_ptr<ms::Session> const&, std::shared_ptr<mrs::Client> const&)> on_connected;

    std::vector<wl_display*> displays;
};

/// Builds the notifier without an `on_client_connected` callback.
class WaylandClientNotifierNoCallbackTest : public WaylandClientNotifierTest
{
public:
    auto make_notification_handler() -> std::unique_ptr<mrs::WaylandServerNotificationHandler> override
    {
        return std::make_unique<mf::WaylandClientNotifier>(shell, authorizer, registry, serial_source, nullptr);
    }
};
}

TEST_F(WaylandClientNotifierTest, allowed_client_is_authorized_with_its_credentials)
{
    std::promise<mf::SessionCredentials> promise;
    EXPECT_CALL(*authorizer, connection_is_allowed(_))
        .WillOnce(DoAll(Invoke([&](mf::SessionCredentials const& creds) { promise.set_value(creds); }), Return(true)));

    inject_client();

    auto future = promise.get_future();
    ASSERT_TRUE(is_ready(future));
    auto const creds = future.get();
    EXPECT_EQ(creds.pid(), ::getpid());
    EXPECT_EQ(creds.uid(), ::getuid());
    EXPECT_EQ(creds.gid(), ::getgid());
}

TEST_F(WaylandClientNotifierTest, allowed_client_opens_a_session_for_its_pid)
{
    std::promise<pid_t> promise;
    EXPECT_CALL(*shell, open_session(_, _, _))
        .WillOnce(Invoke(
            [&](pid_t pid, mir::Fd, std::string const&)
            {
                promise.set_value(pid);
                return std::make_shared<mtd::StubSession>(pid);
            }));

    inject_client();

    auto future = promise.get_future();
    ASSERT_TRUE(is_ready(future));
    EXPECT_EQ(future.get(), ::getpid());
}

TEST_F(WaylandClientNotifierTest, allowed_client_triggers_on_client_connected_with_its_session)
{
    std::promise<std::shared_ptr<ms::Session>> opened;
    EXPECT_CALL(*shell, open_session(_, _, _))
        .WillOnce(Invoke(
            [&](pid_t pid, mir::Fd, std::string const&)
            {
                auto session = std::make_shared<mtd::StubSession>(pid);
                opened.set_value(session);
                return session;
            }));

    auto connection = next_connection();
    auto const server_fd = inject_client().second;

    ASSERT_TRUE(is_ready(connection));
    auto const [socket_fd, session, client] = connection.get();
    EXPECT_EQ(socket_fd, server_fd);
    EXPECT_EQ(session, opened.get_future().get());
    EXPECT_NE(client, nullptr);
}

TEST_F(WaylandClientNotifierTest, allowed_client_is_registered)
{
    auto connection = next_connection();
    inject_client();

    ASSERT_TRUE(is_ready(connection));
    auto const client = connection.get().client;

    auto const registered = on_server_thread([&] { return registry.from(client->raw_client()); });
    EXPECT_EQ(registered, client);
}

TEST_F(WaylandClientNotifierTest, rejected_client_is_not_opened_or_registered)
{
    std::promise<void> authorized;
    EXPECT_CALL(*authorizer, connection_is_allowed(_))
        .WillOnce(DoAll(InvokeWithoutArgs([&] { authorized.set_value(); }), Return(false)));
    EXPECT_CALL(*shell, open_session(_, _, _)).Times(0);

    std::atomic<bool> connected{false};
    on_connected = [&](auto&&...) { connected = true; };

    inject_client();

    auto future = authorized.get_future();
    ASSERT_TRUE(is_ready(future));
    // Barrier: ensure the notifier has finished processing the (rejected) client.
    on_server_thread([] { return true; });
    EXPECT_FALSE(connected);
}

TEST_F(WaylandClientNotifierTest, rejected_client_is_disconnected)
{
    EXPECT_CALL(*authorizer, connection_is_allowed(_)).WillOnce(Return(false));

    auto* const display = inject_client().first;

    // The notifier kills a rejected client, which the server observes as a
    // disconnect. (wayland-rs tears the connection down rather than delivering a
    // wire `wl_display.error`, so the client sees a connection error, not a
    // protocol error.)
    ASSERT_TRUE(wait_for_server_disconnect(display)) << "The rejected client was not disconnected";
    EXPECT_NE(wl_display_get_error(display), 0);
}

TEST_F(WaylandClientNotifierNoCallbackTest, allowed_client_is_still_opened_without_a_callback)
{
    std::promise<void> opened;
    EXPECT_CALL(*shell, open_session(_, _, _))
        .WillOnce(Invoke(
            [&](pid_t pid, mir::Fd, std::string const&)
            {
                opened.set_value();
                return std::make_shared<mtd::StubSession>(pid);
            }));

    auto* const display = inject_client().first;

    auto future = opened.get_future();
    ASSERT_TRUE(is_ready(future));

    // The client remains a fully-functional, non-killed connection.
    EXPECT_NE(wl_display_roundtrip(display), -1);
    EXPECT_EQ(wl_display_get_error(display), 0);
}

TEST_F(WaylandClientNotifierTest, removed_client_closes_its_session_and_is_deregistered)
{
    std::promise<std::shared_ptr<ms::Session>> closed;
    EXPECT_CALL(*shell, close_session(_))
        .WillOnce(Invoke([&](std::shared_ptr<ms::Session> const& session) { closed.set_value(session); }));

    auto connection = next_connection();
    auto* const display = inject_client().first;

    ASSERT_TRUE(is_ready(connection));
    auto const [socket_fd, session, client] = connection.get();

    // Disconnecting fires the Rust destroy listener, driving `client_removed`.
    wl_display_disconnect(display);
    std::erase(displays, display);

    auto closed_future = closed.get_future();
    ASSERT_TRUE(is_ready(closed_future));
    EXPECT_EQ(closed_future.get(), session);

    auto const registered = on_server_thread([&] { return registry.from(client->raw_client()); });
    EXPECT_EQ(registered, nullptr);
}
