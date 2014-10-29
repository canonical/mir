/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
using namespace ::testing;

mtf::InterprocessClientServerTest::~InterprocessClientServerTest()
{
    if (getpid() != test_process_id)
    {
        auto const status = ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
        exit(status);
    }
}

void mtf::InterprocessClientServerTest::init_server(std::function<void()> const& init_code)
{
    server_setup = init_code;
}

void mtf::InterprocessClientServerTest::run_in_server(std::function<void()> const& exec_code)
{
    if (test_process_id != getpid()) return;

    CrossProcessSync started_sync;

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
    if (test_process_id != getpid()) return;

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
    }
    else
    {
        client_process_id = pid;
        auto const client_process = std::make_shared<Process>(pid);
        Result result = client_process->wait_for_termination();
        EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
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
        shutdown_sync.wait_for_signal_ready_for();
        stop_server();
    }

    if (test_process_id != getpid()) return;

    shutdown_sync.signal_ready();

    if (server_process)
    {
        Result result = server_process->wait_for_termination();
        server_process.reset();
        EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
    }
}
