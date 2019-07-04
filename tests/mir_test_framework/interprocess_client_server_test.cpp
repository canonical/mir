/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/process.h"
#include "mir/test/death.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
using namespace ::testing;

mtf::InterprocessClientServerTest::~InterprocessClientServerTest()
{
    if (getpid() != test_process_id)
    {
        shutdown_sync.reset();
        auto const status = ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
        exit(status);
    }
}

void mtf::InterprocessClientServerTest::init_server(std::function<void()> const& init_code)
{
    if (auto const& existing = server_setup)
    {
        server_setup = [=]
            {
                existing();
                init_code();
            };
    }
    else
    {
        server_setup = init_code;
    }
}

void mtf::InterprocessClientServerTest::run_in_server_and_disable_core_dump(
        std::function<void()> const& exec_code)
{
    run_in_server([exec_code]()
    {
        mir::test::disable_core_dump();
        exec_code();
    });
}

void mtf::InterprocessClientServerTest::run_in_server(std::function<void()> const& exec_code)
{
    if (test_process_id != getpid()) return;

    mt::CrossProcessSync started_sync;

    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        server_process_id = getpid();
        process_tag = "server";
        add_to_environment("MIR_SERVER_FILE", mir_test_socket);
	add_to_environment("MIR_SERVER_ENABLE_MIRCLIENT", "");
                
        server_setup();
        start_server();
        started_sync.signal_ready();
        exec_code();
    }
    else
    {
        server_process = std::make_shared<Process>(pid);
        started_sync.wait_for_signal_ready_for();
    }
}

void mtf::InterprocessClientServerTest::run_in_client(std::function<void()> const& client_code)
{
    auto const client_process = new_client_process(client_code);

    if (test_process_id != getpid()) return;

    Result result = client_process->wait_for_termination();
    EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
}

auto mtf::InterprocessClientServerTest::new_client_process(std::function<void()> const& client_code)
-> std::shared_ptr<Process>
{
    if (test_process_id != getpid())
        return std::shared_ptr<Process>{};

    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        process_tag = "client";
        add_to_environment("MIR_SOCKET", mir_test_socket);
        client_code();
        return std::shared_ptr<Process>{};
    }
    else
    {
        client_process_id = pid;
        return std::make_shared<Process>(pid);
    }
}

bool mtf::InterprocessClientServerTest::is_test_process() const
{
    return test_process_id == getpid();
}

void mtf::InterprocessClientServerTest::TearDown()
{
    if (server_process_id == getpid())
    {
        shutdown_sync->wait_for_signal_ready_for();
    }

    stop_server();
}

void mtf::InterprocessClientServerTest::stop_server()
{
    if (server_process_id == getpid())
    {
        HeadlessTest::stop_server();
    }

    if (test_process_id != getpid()) return;

    shutdown_sync->signal_ready();

    if (server_process)
    {
        Result result = server_process->wait_for_termination();
        server_process.reset();

        if (!server_signal_expected)
        {
            EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
        }
        else
        {
            EXPECT_THAT(result.signalled(), Eq(true));
            // FIXME Under valgrind we always see SIGKILL (so treat that as a pass)
            EXPECT_THAT(result.signal, AnyOf(Eq(expected_server_failure_signal), Eq(SIGKILL)));
        }
    }
}

void mtf::InterprocessClientServerTest::expect_server_signalled(int signal)
{
    server_signal_expected = true;
    expected_server_failure_signal = signal;
}

bool mtf::InterprocessClientServerTest::sigkill_server_process()
{
    if (test_process_id != getpid())
        throw std::logic_error("Only the test process may kill the server");

    if (!server_process)
        throw std::logic_error("No server process to kill");

    server_process->kill();
    auto result = wait_for_shutdown_server_process();

    return result.reason == TerminationReason::child_terminated_by_signal &&
           result.signal == SIGKILL;
}

mtf::Result mtf::InterprocessClientServerTest::wait_for_shutdown_server_process()
{

    if (!server_process)
        throw std::logic_error("No server process to monitor");

    Result result = server_process->wait_for_termination();
    server_process.reset();
    return result;
}
