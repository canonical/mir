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

#include "mir/process/process.h"
#include "mir/process/signal_dispatcher.h"

#include <gtest/gtest.h>

namespace mp = mir::process;

namespace
{

struct SignalCollector
{
    SignalCollector() : signal(mp::Result::invalid_signal)
    {
    }
    
    void operator()(int s)
    {
        signal = s;
    }
    
    int signal;
};

void a_main_function_accessing_the_signal_dispatcher()
{
    // Ensure that the SignalDispatcher has been created.
    mp::SignalDispatcher::instance();
}

template<int signal>
void a_main_function_collecting_received_signals()
{
    mp::SignalDispatcher::instance()->enable_for(signal);
    SignalCollector sc;
    boost::signals2::scoped_connection conn(
        mp::SignalDispatcher::instance()->signal_channel().connect(sc));

    ::kill(getpid(), signal);
    
    EXPECT_EQ(signal, sc.signal);
}

int a_successful_exit_function()
{
    return EXIT_SUCCESS; 
}

int a_gtest_exit_function()
{
    return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
}

}

TEST(SignalDispatcher,
     a_default_dispatcher_does_not_catch_any_signals)
{
    auto p = mp::fork_and_run_in_a_different_process(
        a_main_function_accessing_the_signal_dispatcher,
        a_successful_exit_function);
    
    p->terminate();

    EXPECT_TRUE(p->wait_for_termination().signalled());
}

TEST(SignalDispatcher,
     enabling_a_signal_results_in_signal_channel_delivering_the_signal)
{
    auto p = mp::fork_and_run_in_a_different_process(
        a_main_function_collecting_received_signals<SIGTERM>,
        a_successful_exit_function);
    
    EXPECT_TRUE(p->wait_for_termination().succeeded());

    p = mp::fork_and_run_in_a_different_process(
        a_main_function_collecting_received_signals<SIGINT>,
        a_successful_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());
}
