/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/glib_main_loop.h"
#include "mir_test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace mt = mir::test;

struct GLibMainLoopTest : ::testing::Test
{
    mir::GLibMainLoop ml;
};

TEST_F(GLibMainLoopTest, stops_from_within_handler)
{
    mt::Signal loop_finished;

    std::thread{
        [&]
        {
            int const owner{0};
            ml.enqueue(&owner, [&] { ml.stop(); });
            ml.run();
            loop_finished.raise();
        }}.detach();

    EXPECT_TRUE(loop_finished.wait_for(std::chrono::seconds{5}));
}

TEST_F(GLibMainLoopTest, stops_from_outside_handler)
{
    mt::Signal loop_running;
    mt::Signal loop_finished;

    std::thread{
        [&]
        {
            int const owner{0};
            ml.enqueue(&owner, [&] { loop_running.raise(); });
            ml.run();
            loop_finished.raise();
        }}.detach();

    ASSERT_TRUE(loop_running.wait_for(std::chrono::seconds{5}));

    ml.stop();

    EXPECT_TRUE(loop_finished.wait_for(std::chrono::seconds{5}));
}

TEST_F(GLibMainLoopTest, ignores_handler_added_after_stop)
{
    int const owner{0};
    bool handler_called{false};
    mt::Signal loop_running;

    std::thread t{
        [&]
        {
            loop_running.wait();
            ml.stop();
            int const owner1{0};
            ml.enqueue(&owner1, [&] { handler_called = true; });
        }};

    ml.enqueue(&owner, [&] { loop_running.raise(); });
    ml.run();

    t.join();

    EXPECT_FALSE(handler_called);
}

TEST_F(GLibMainLoopTest, handles_signal)
{
    int const signum{SIGUSR1};
    int handled_signum{0};

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
           handled_signum = sig;
           ml.stop();
        });

    kill(getpid(), signum);

    ml.run();

    ASSERT_EQ(signum, handled_signum);
}

TEST_F(GLibMainLoopTest, handles_multiple_signals)
{
    std::vector<int> const signals{SIGUSR1, SIGUSR2};
    size_t const num_signals_to_send{10};
    std::vector<int> handled_signals;
    std::atomic<unsigned int> num_handled_signals{0};

    ml.register_signal_handler(
        {signals[0], signals[1]},
        [&handled_signals, &num_handled_signals](int sig)
        {
           handled_signals.push_back(sig);
           ++num_handled_signals;
        });


    std::thread signal_sending_thread(
        [this, num_signals_to_send, &signals, &num_handled_signals]
        {
            for (size_t i = 0; i < num_signals_to_send; i++)
            {
                kill(getpid(), signals[i % signals.size()]);
                while (num_handled_signals <= i) std::this_thread::yield();
            }
            ml.stop();
        });

    ml.run();

    signal_sending_thread.join();

    ASSERT_EQ(num_signals_to_send, handled_signals.size());

    for (size_t i = 0; i < num_signals_to_send; i++)
        ASSERT_EQ(signals[i % signals.size()], handled_signals[i]) << " index " << i;
}

TEST_F(GLibMainLoopTest, invokes_all_registered_handlers_for_signal)
{
    int const signum{SIGUSR1};
    std::vector<int> handled_signum{0,0,0};

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[0] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[1] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    ml.register_signal_handler(
        {signum},
        [&handled_signum, this](int sig)
        {
            handled_signum[2] = sig;
            if (handled_signum[0] != 0 &&
                handled_signum[1] != 0 &&
                handled_signum[2] != 0)
            {
                ml.stop();
            }
        });

    kill(getpid(), signum);

    ml.run();

    ASSERT_EQ(signum, handled_signum[0]);
    ASSERT_EQ(signum, handled_signum[1]);
    ASSERT_EQ(signum, handled_signum[2]);
}
