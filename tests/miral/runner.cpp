/*
 * Copyright © 2016-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/test_server.h"
#include "miral/x11_support.h"

#include "mir/test/pipe.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace mt = mir::test;

namespace
{
auto const a_long_time = std::chrono::seconds(1);

struct Runner : miral::TestServer
{
    void SetUp() override
    {
    }

    MOCK_METHOD0(callback, void());

    void wait_for_locking_callback()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ready_to_tear_down = false;
    }

    void locking_callback()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ready_to_tear_down = true;
        }

        cv.notify_one();
        callback();
    }

    void wait_to_tear_down()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return ready_to_tear_down; });
        lock.unlock();
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool ready_to_tear_down = true;
    mt::Pipe pipe;
    char const data_to_write = 'a';
};
}

TEST_F(Runner, stop_callback_is_called)
{
    add_stop_callback([this] { callback(); });
    miral::TestServer::SetUp();
    EXPECT_CALL(*this, callback());
}

TEST_F(Runner, start_callback_is_called)
{
    mt::Signal signal;

    add_start_callback([this] { callback(); });
    EXPECT_CALL(*this, callback())
        .WillOnce(InvokeWithoutArgs([&] { signal.raise(); }));

    miral::TestServer::SetUp();

    signal.wait_for(a_long_time);
    testing::Mock::VerifyAndClearExpectations(this);
}

TEST_F(Runner, wayland_socket_is_returned_by_default)
{
    miral::TestServer::SetUp();

    miral::TestServer::invoke_runner([](auto& runner){ EXPECT_TRUE(runner.wayland_display().is_set()); });
}

TEST_F(Runner, x11_socket_is_not_returned_by_default)
{
    miral::TestServer::SetUp();

    miral::TestServer::invoke_runner([](auto& runner){ EXPECT_FALSE(runner.x11_display().is_set()); });
}

TEST_F(Runner, register_signal_handler_before_setup_invokes_callback_after_setup)
{
    int const signum{SIGUSR1};

    wait_for_locking_callback();
    register_signal_handler({signum}, [this](int){ locking_callback(); });

    miral::TestServer::SetUp();

    EXPECT_CALL(*this, callback())
        .Times(AtLeast(1));

    kill(getpid(), signum);
    wait_to_tear_down();
}

TEST_F(Runner, register_signal_handler_after_setup_invokes_callback_when_signal_raised)
{
    miral::TestServer::SetUp();

    int const signum{SIGUSR1};

    wait_for_locking_callback();
    register_signal_handler({signum}, [this](int){ locking_callback(); });

    EXPECT_CALL(*this, callback())
        .Times(AtLeast(1));

    kill(getpid(), signum);
    wait_to_tear_down();
}

TEST_F(Runner, register_fd_handler_before_setup_invokes_callback_after_setup)
{
    wait_for_locking_callback();
    auto const handle = register_fd_handler(pipe.read_fd(), [this](int){ locking_callback(); });

    miral::TestServer::SetUp();

    EXPECT_CALL(*this, callback())
        .Times(AtLeast(1));
    
    write(pipe.write_fd(), &data_to_write, sizeof(data_to_write));
    wait_to_tear_down();
}

TEST_F(Runner, register_fd_handler_after_setup_invokes_callback_when_fd_written_to)
{
    miral::TestServer::SetUp();

    wait_for_locking_callback();
    auto const handle = register_fd_handler(pipe.read_fd(), [this](int){ locking_callback(); });

    EXPECT_CALL(*this, callback())
        .Times(AtLeast(1));

    write(pipe.write_fd(), &data_to_write, sizeof(data_to_write));
    wait_to_tear_down();
}

// We can't spin up the X11 subsystem during LP builds. We would get:
// "Mir fatal error: Failed to bind socket /tmp/.X11-unix/X666"
//TEST_F(Runner, x11_socket_is_returned_if_configured)
//{
//    miral::X11Support x11support;
//    add_server_init(x11support);
//    add_to_environment("MIR_SERVER_x11_DISPLAY_EXPERIMENTAL", "666");
//
//    miral::TestServer::SetUp();
//
//    miral::TestServer::invoke_runner([](auto& runner)
//    {
//        EXPECT_TRUE(runner.x11_display().is_set());
//        EXPECT_THAT(runner.x11_display().value(), Eq(":666"));
//    });
//}
