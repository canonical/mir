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
#include <mir_test_framework/temporary_environment_value.h>

#include <gtest/gtest.h>

#include <condition_variable>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <unistd.h>

namespace mrs = mir::wayland_rs;
namespace mtf = mir_test_framework;

using namespace std::chrono_literals;

namespace
{
/// A notification handler that ignores everything. These tests do not care
/// about client lifecycle events.
class StubNotificationHandler : public mrs::WaylandServerNotificationHandler
{
public:
    void client_added(rust::Box<mrs::WaylandClient>) override {}
    void client_removed(rust::Box<mrs::WaylandClientId>) override {}
};

/// An `FdReadyCallback` that counts how often it is notified and signals a
/// condition variable so a test can wait for the notification. It drains the
/// watched descriptor on each notification because the event loop is
/// level-triggered (an undrained descriptor would be reported ready
/// repeatedly).
class CountingFdReadyCallback : public mrs::FdReadyCallback
{
public:
    CountingFdReadyCallback(int fd, std::mutex& mutex, std::condition_variable& cv, int& count)
        : fd{fd}, mutex{mutex}, cv{cv}, count{count}
    {
    }

    void ready() override
    {
        char buffer;
        while (::read(fd, &buffer, sizeof(buffer)) > 0)
        {
        }

        std::lock_guard<std::mutex> lock{mutex};
        ++count;
        cv.notify_one();
    }

private:
    int const fd;
    std::mutex& mutex;
    std::condition_variable& cv;
    int& count;
};

/// Create a fresh, private temporary directory for use as XDG_RUNTIME_DIR (the
/// Wayland server binds its listening socket there). Returns its path.
auto make_temp_runtime_dir() -> std::string
{
    char template_path[] = "/tmp/mir-wayland-rs-test-XXXXXX";
    auto const dir = ::mkdtemp(template_path);
    if (!dir)
        throw std::runtime_error{"Failed to create temporary XDG_RUNTIME_DIR"};

    ::chmod(dir, 0700);
    return dir;
}

class FdReadyListenerTest : public testing::Test
{
public:
    void SetUp() override
    {
        // Always run against a private temporary XDG_RUNTIME_DIR so the test
        // never pollutes (or collides with) a real runtime directory. It is
        // removed again in TearDown.
        runtime_dir = make_temp_runtime_dir();
        xdg_runtime_dir.emplace("XDG_RUNTIME_DIR", runtime_dir.c_str());

        if (::pipe2(pipe_fds, O_NONBLOCK) != 0)
            throw std::runtime_error{"Failed to create pipe"};

        server.emplace(mrs::create_wayland_server());
        auto executor_owned = std::make_unique<mrs::WaylandExecutor>(**server);

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
    }

    void TearDown() override
    {
        if (server)
            (*server)->stop();
        if (server_thread.joinable())
            server_thread.join();

        if (pipe_fds[0] >= 0)
            ::close(pipe_fds[0]);
        if (pipe_fds[1] >= 0)
            ::close(pipe_fds[1]);

        // Restore the previous environment and remove the temporary directory
        // (along with the socket the server bound inside it).
        xdg_runtime_dir.reset();

        if (!runtime_dir.empty())
        {
            std::error_code ec;
            std::filesystem::remove_all(runtime_dir, ec);
        }
    }

    auto read_fd() const -> int { return pipe_fds[0]; }
    auto write_fd() const -> int { return pipe_fds[1]; }

    void write_byte()
    {
        char const byte = 1;
        ASSERT_EQ(::write(write_fd(), &byte, sizeof(byte)), static_cast<ssize_t>(sizeof(byte)));
    }

    std::string runtime_dir;
    std::optional<mtf::TemporaryEnvironmentValue> xdg_runtime_dir;
    std::optional<rust::Box<mrs::WaylandServer>> server;
    std::thread server_thread;
    int pipe_fds[2]{-1, -1};
};
}

TEST_F(FdReadyListenerTest, notifies_when_fd_becomes_readable)
{
    std::mutex mutex;
    std::condition_variable cv;
    int count{0};

    (*server)->register_fd_ready_listener(
        read_fd(),
        std::make_unique<CountingFdReadyCallback>(read_fd(), mutex, cv, count));

    write_byte();

    std::unique_lock<std::mutex> lock{mutex};
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return count > 0; }))
        << "Listener was not notified when the fd became readable";
}

TEST_F(FdReadyListenerTest, does_not_notify_when_fd_not_written)
{
    std::mutex mutex;
    std::condition_variable cv;
    int count{0};

    (*server)->register_fd_ready_listener(
        read_fd(),
        std::make_unique<CountingFdReadyCallback>(read_fd(), mutex, cv, count));

    // Never write to the pipe; the listener must not fire.
    std::unique_lock<std::mutex> lock{mutex};
    EXPECT_FALSE(cv.wait_for(lock, 500ms, [&] { return count > 0; }))
        << "Listener was notified despite the fd never becoming readable";
}

TEST_F(FdReadyListenerTest, notifies_again_after_subsequent_write)
{
    std::mutex mutex;
    std::condition_variable cv;
    int count{0};

    (*server)->register_fd_ready_listener(
        read_fd(),
        std::make_unique<CountingFdReadyCallback>(read_fd(), mutex, cv, count));

    write_byte();
    {
        std::unique_lock<std::mutex> lock{mutex};
        ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return count >= 1; }))
            << "Listener was not notified on the first write";
    }

    write_byte();
    {
        std::unique_lock<std::mutex> lock{mutex};
        ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return count >= 2; }))
            << "Listener was not notified on the second write";
    }
}
