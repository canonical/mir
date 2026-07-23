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

#ifndef MIR_TESTS_WAYLAND_RS_SERVER_TEST_H
#define MIR_TESTS_WAYLAND_RS_SERVER_TEST_H

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_executor.h"

#include <mir/executor.h>
#include <mir_test_framework/temporary_environment_value.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>

namespace mir
{
namespace wayland
{
namespace test
{
/// A notification handler that ignores everything. These server tests do not
/// care about client lifecycle events.
class StubNotificationHandler : public WaylandServerNotificationHandler
{
public:
    void client_added(rust::Box<WaylandClient>) override {}
    void client_removed(rust::Box<WaylandClientId>) override {}
};

/// Create a fresh, private temporary directory for use as XDG_RUNTIME_DIR (the
/// Wayland server binds its listening socket there). Returns its path.
inline auto make_temp_runtime_dir() -> std::string
{
    char template_path[] = "/tmp/mir-wayland-rs-test-XXXXXX";
    auto const dir = ::mkdtemp(template_path);
    if (!dir)
        throw std::runtime_error{"Failed to create temporary XDG_RUNTIME_DIR"};

    ::chmod(dir, 0700);
    return dir;
}

class WaylandWorkCallback : public mir::wayland::WorkCallback
{
public:
    explicit WaylandWorkCallback(std::shared_ptr<mir::wayland::WaylandExecutor> const& executor)
        : executor(executor)
    {
    }

    void execute() override
    {
        if (auto const locked = executor.lock())
            locked->execute();
    }

private:
    std::weak_ptr<mir::wayland::WaylandExecutor> executor;
};

/// A test fixture that runs a `WaylandServer` on a background thread.
///
/// The server is bound inside a private temporary XDG_RUNTIME_DIR (so the test
/// never pollutes or collides with a real runtime directory) with a stub
/// notification handler. A `WaylandExecutor`, exposed via `executor`, lets a
/// test schedule work onto the server's event loop. The server is stopped and
/// joined, and the temporary directory removed, in `TearDown`.
class RunningWaylandServerTest : public testing::Test
{
public:
    void SetUp() override
    {
        runtime_dir = make_temp_runtime_dir();
        xdg_runtime_dir.emplace("XDG_RUNTIME_DIR", runtime_dir.c_str());

        server.emplace(create_wayland_server());
        auto executor = std::make_shared<WaylandExecutor>(**server);

        // A socket name unique to this process so parallel test runs do not collide.
        socket = "mir-wayland-rs-test-" + std::to_string(::getpid());

        server_thread = std::thread{
            [this,
             notification_handler = make_notification_handler(),
             executor = executor]() mutable
            {
                (*server)->run(
                    socket,
                    nullptr,
                    std::move(notification_handler),
                    std::make_unique<WaylandWorkCallback>(executor));
            }};
    }

    /// The notification handler the server is run with. Override to observe
    /// client lifecycle events; defaults to a handler that ignores everything.
    virtual auto make_notification_handler() -> std::unique_ptr<WaylandServerNotificationHandler>
    {
        return std::make_unique<StubNotificationHandler>();
    }

    void TearDown() override
    {
        if (server)
            (*server)->stop();
        if (server_thread.joinable())
            server_thread.join();

        // Restore the previous environment and remove the temporary directory
        // (along with the socket the server bound inside it).
        xdg_runtime_dir.reset();

        if (!runtime_dir.empty())
        {
            std::error_code ec;
            std::filesystem::remove_all(runtime_dir, ec);
        }
    }

    std::string runtime_dir;
    std::optional<mir_test_framework::TemporaryEnvironmentValue> xdg_runtime_dir;
    std::optional<rust::Box<WaylandServer>> server;
    mir::Executor* executor{nullptr};
    std::string socket;
    std::thread server_thread;
};
}
}
}

#endif  // MIR_TESTS_WAYLAND_RS_SERVER_TEST_H
