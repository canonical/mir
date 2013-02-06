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

#include "mir_test_framework/process.h"

#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

namespace mir
{

struct MainFunctionFactory
{
    static void an_empty_main_function()
    {
    }

    static void an_infinitely_waiting_main_function()
    {
        std::mutex m;
        std::unique_lock<std::mutex> ul(m);

        std::condition_variable cv;

        cv.wait(ul);
    }

    static void a_value_altering_main_function(
        int& value,
        int expected_value_after_increment)
    {

        value++;

        EXPECT_EQ(
            expected_value_after_increment,
            value);
    }
};

struct ExitFunctionFactory
{
    static int a_successful_exit_function()
    {
        return EXIT_SUCCESS;
    }

    static int a_failing_exit_function()
    {
        return EXIT_FAILURE;
    }

    static int a_gtest_exit_function()
    {
        return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
    }
};

TEST(ProcessResult,
     a_default_result_never_succeeds)
{
    mtf::Result r;

    EXPECT_FALSE(r.succeeded());
}

TEST(ProcessResult,
     a_signalled_result_does_not_succeed)
{
    mtf::Result r;
    r.reason = mtf::TerminationReason::child_terminated_by_signal;

    EXPECT_FALSE(r.succeeded());
}

TEST(ProcessResult,
     a_normally_terminated_result_succeeds_only_with_exit_success)
{
    mtf::Result r;
    r.reason = mtf::TerminationReason::child_terminated_normally;
    r.exit_code = EXIT_FAILURE;
    EXPECT_FALSE(r.succeeded());
    r.exit_code = EXIT_SUCCESS;
    EXPECT_TRUE(r.succeeded());
}

TEST(Process,
     a_main_fn_is_executed)
{
    int value = 0;

    auto p = mtf::fork_and_run_in_a_different_process(
        std::bind(
            MainFunctionFactory::a_value_altering_main_function,
            value,
            1),
        ExitFunctionFactory::a_gtest_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());
}

TEST(Process,
     a_successful_exit_function_succeeds)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        MainFunctionFactory::an_empty_main_function,
        ExitFunctionFactory::a_successful_exit_function);

    EXPECT_TRUE(p->wait_for_termination().succeeded());
}

TEST(Process,
     a_failing_exit_function_does_not_succeed)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        MainFunctionFactory::an_empty_main_function,
        ExitFunctionFactory::a_failing_exit_function);

    EXPECT_FALSE(p->wait_for_termination().succeeded());
}

TEST(Process,
     a_terminated_child_is_recognized_as_being_signalled)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        MainFunctionFactory::an_infinitely_waiting_main_function,
        ExitFunctionFactory::a_successful_exit_function);

    p->terminate();

    EXPECT_TRUE(p->wait_for_termination().signalled());
}

TEST(Process,
     a_killed_child_is_recognized_as_being_signalled)
{
    auto p = mtf::fork_and_run_in_a_different_process(
        MainFunctionFactory::an_infinitely_waiting_main_function,
        ExitFunctionFactory::a_successful_exit_function);

    p->kill();

    EXPECT_TRUE(p->wait_for_termination().signalled());
}
}
