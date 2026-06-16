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

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_executor.h"

#include <mir/executor.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace mrs = mir::wayland_rs;

using namespace std::chrono_literals;

namespace
{
/// A notification handler that ignores everything. The executor tests do not
/// care about client lifecycle events.
class StubNotificationHandler : public mrs::WaylandServerNotificationHandler
{
public:
    void client_added(rust::Box<mrs::WaylandClient>) override {}
    void client_removed(rust::Box<mrs::WaylandClientId>) override {}
};

/// Ensure XDG_RUNTIME_DIR points at a usable directory, as the Wayland server
/// binds its listening socket there. Returns the directory in use.
auto ensure_xdg_runtime_dir() -> std::string
{
    if (auto const existing = ::getenv("XDG_RUNTIME_DIR"))
        return existing;

    char template_path[] = "/tmp/mir-wayland-rs-test-XXXXXX";
    auto const dir = ::mkdtemp(template_path);
    if (!dir)
        throw std::runtime_error{"Failed to create temporary XDG_RUNTIME_DIR"};

    ::chmod(dir, 0700);
    ::setenv("XDG_RUNTIME_DIR", dir, 1);
    return dir;
}

class WaylandExecutorTest : public testing::Test
{
public:
    void SetUp() override
    {
        ensure_xdg_runtime_dir();

        server.emplace(mrs::create_wayland_server());
        auto executor_owned = std::make_unique<mrs::WaylandExecutor>(**server);
        executor = executor_owned.get();

        // A socket name unique to this process so parallel test runs do not collide.
        auto const socket = "mir-wayland-rs-test-" + std::to_string(::getpid());

        server_thread = std::thread{
            [this, socket, handler = std::move(executor_owned)]() mutable
            {
                (*server)->run(
                    socket,
                    nullptr,
                    std::make_unique<StubNotificationHandler>(),
                    std::move(handler));
            }};

        ASSERT_TRUE(wait_until_ready(5s)) << "Wayland server did not become ready in time";
    }

    void TearDown() override
    {
        if (server)
            (*server)->stop();
        if (server_thread.joinable())
            server_thread.join();
    }

    /// Spawn probe work repeatedly until one is actually executed, indicating
    /// the server's event loop is running and its work queue is wired up.
    ///
    /// Work spawned before the queue exists is silently dropped, so this retries
    /// rather than relying on a single probe.
    auto wait_until_ready(std::chrono::milliseconds timeout) -> bool
    {
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            auto ran = std::make_shared<std::atomic<bool>>(false);
            executor->spawn([ran] { ran->store(true); });

            auto const probe_deadline = std::chrono::steady_clock::now() + 50ms;
            while (std::chrono::steady_clock::now() < probe_deadline)
            {
                if (ran->load())
                    return true;
                std::this_thread::sleep_for(2ms);
            }
        }
        return false;
    }

    std::optional<rust::Box<mrs::WaylandServer>> server;
    mir::Executor* executor{nullptr};
    std::thread server_thread;
};
}

TEST_F(WaylandExecutorTest, runs_scheduled_work)
{
    std::mutex mutex;
    std::condition_variable cv;
    bool ran{false};

    executor->spawn(
        [&]
        {
            std::lock_guard<std::mutex> lock{mutex};
            ran = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return ran; }))
        << "Scheduled work was not executed";
}

TEST_F(WaylandExecutorTest, runs_multiple_scheduled_work_in_order)
{
    int const num_work = 16;

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<int> order;

    for (int i = 0; i < num_work; ++i)
    {
        executor->spawn(
            [&, i]
            {
                std::lock_guard<std::mutex> lock{mutex};
                order.push_back(i);
                cv.notify_one();
            });
    }

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return order.size() == static_cast<size_t>(num_work); }))
        << "Not all scheduled work was executed";

    std::vector<int> expected(num_work);
    for (int i = 0; i < num_work; ++i)
        expected[i] = i;

    EXPECT_EQ(order, expected);
}

TEST_F(WaylandExecutorTest, runs_work_on_event_loop_thread_not_caller)
{
    std::mutex mutex;
    std::condition_variable cv;
    bool ran{false};
    std::thread::id work_thread;

    auto const caller_thread = std::this_thread::get_id();

    executor->spawn(
        [&]
        {
            std::lock_guard<std::mutex> lock{mutex};
            work_thread = std::this_thread::get_id();
            ran = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return ran; }))
        << "Scheduled work was not executed";

    EXPECT_NE(work_thread, caller_thread);
}
