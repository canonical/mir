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

#include <gtest/gtest.h>

#include <condition_variable>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unistd.h>

namespace mrs = mir::wayland_rs;

using namespace std::chrono_literals;

namespace
{
/// An `FdReadyCallback` that counts how often it is notified and signals a
/// condition variable so a test can wait for the notification. The registration
/// is one-shot, so it expects to be notified at most once; it drains the
/// watched descriptor purely to consume the pending byte.
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

class FdReadyListenerTest : public mrs::test::RunningWaylandServerTest
{
public:
    void SetUp() override
    {
        if (::pipe2(pipe_fds, O_NONBLOCK) != 0)
            throw std::runtime_error{"Failed to create pipe"};

        RunningWaylandServerTest::SetUp();
    }

    void TearDown() override
    {
        RunningWaylandServerTest::TearDown();

        if (pipe_fds[0] >= 0)
            ::close(pipe_fds[0]);
        if (pipe_fds[1] >= 0)
            ::close(pipe_fds[1]);
    }

    auto read_fd() const -> int { return pipe_fds[0]; }
    auto write_fd() const -> int { return pipe_fds[1]; }

    void write_byte()
    {
        char const byte = 1;
        ASSERT_EQ(::write(write_fd(), &byte, sizeof(byte)), static_cast<ssize_t>(sizeof(byte)));
    }

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
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return count >= 1; }))
        << "Listener was not notified when the fd became readable";

    // The registration is one-shot, so it must fire exactly once.
    EXPECT_EQ(count, 1);
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
    EXPECT_FALSE(cv.wait_for(lock, 500ms, [&] { return count >= 1; }))
        << "Listener was notified despite the fd never becoming readable";
    EXPECT_EQ(count, 0);
}

TEST_F(FdReadyListenerTest, only_notifies_once_for_a_one_shot_registration)
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
        EXPECT_EQ(count, 1);
    }

    // The registration is one-shot: the descriptor is no longer watched, so a
    // second write must not produce a further notification.
    write_byte();
    {
        std::unique_lock<std::mutex> lock{mutex};
        EXPECT_FALSE(cv.wait_for(lock, 500ms, [&] { return count >= 2; }))
            << "Listener was notified again after the one-shot registration fired";
        EXPECT_EQ(count, 1);
    }
}
