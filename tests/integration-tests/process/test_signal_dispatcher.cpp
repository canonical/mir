/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/signal_dispatcher.h"
#include "mir_test_framework/process.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

struct SignalCollector
{
    SignalCollector() : signal(-1)
    {
    }

    void operator()(int s)
    {
        std::unique_lock<std::mutex> lock(mutex);
        signal = s;
        cv.notify_all();
    }

    int load()
    {
        unsigned int counter = 0;
        std::unique_lock<std::mutex> lock(mutex);
        while (signal == -1 && counter < 100)
        {
            cv.wait_for(lock, std::chrono::milliseconds(10));
            counter++;
        }
        return signal;
    }

    int signal;

    std::mutex mutex;
    std::condition_variable cv;

    SignalCollector(SignalCollector const&) = delete;
};

void a_main_function_accessing_the_signal_dispatcher()
{
    // Ensure that the SignalDispatcher has been created.
    mir::SignalDispatcher::instance();
    // Don't return (and exit) before the parent process has a chance to signal
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

template<int signal>
void a_main_function_collecting_received_signals()
{
    mir::SignalDispatcher::instance()->enable_for(signal);
    SignalCollector sc;
    boost::signals2::scoped_connection conn(
        mir::SignalDispatcher::instance()->signal_channel().connect(boost::ref(sc)));

    ::kill(getpid(), signal);

    EXPECT_EQ(signal, sc.load());
}

int a_successful_exit_function()
{
    return EXIT_SUCCESS;
}

int a_gtest_exit_function()
{
    return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
}


TEST(SignalDispatcher,
    DISABLED_a_default_dispatcher_does_not_catch_any_signals)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        a_main_function_accessing_the_signal_dispatcher,
        a_successful_exit_function);

    p->terminate();

    EXPECT_TRUE(p->wait_for_termination().signalled());
}

TEST(SignalDispatcher,
    DISABLED_enabling_a_signal_results_in_signal_channel_delivering_the_signal)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        a_main_function_collecting_received_signals<SIGTERM>,
        a_successful_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());

    p = mtf::fork_and_run_in_a_different_process(
        a_main_function_collecting_received_signals<SIGINT>,
        a_successful_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());
}
