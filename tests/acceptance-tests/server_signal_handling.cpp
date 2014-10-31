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

#include "mir/server.h"

#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir_test_framework/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <csignal>

#include <chrono>

namespace mtf = mir_test_framework;

namespace
{
struct ServerSignal : mtf::InterprocessClientServerTest
{
    std::chrono::milliseconds const timeout{200};
    MOCK_CONST_METHOD1(terminate_handler, void(int));
    mtf::CrossProcessSync cleanup_done;

    void SetUp() override
    {
        init_server([&]
           {
            server.set_terminator([&](int signal)
                {
                    terminate_handler(signal);
                    server.stop();
                });

            server.add_emergency_cleanup([&]
                { cleanup_done.signal_ready(); });
           });
    }
};
}

TEST_F(ServerSignal, terminate_handler_is_called_for_SIGTERM)
{
    run_in_server([&]
        {
            EXPECT_CALL(*this, terminate_handler(SIGTERM));
            kill(getpid(), SIGTERM);
            wait_for_server_exit();
        });
}

TEST_F(ServerSignal, terminate_handler_is_called_for_SIGINT)
{
    run_in_server([&]
        {
            EXPECT_CALL(*this, terminate_handler(SIGINT));
            kill(getpid(), SIGINT);
            wait_for_server_exit();
        });
}

TEST_F(ServerSignal, cleanup_handler_is_called_for_SIGABRT)
{
    expect_server_signalled(SIGABRT);

    run_in_server([&]{ kill(getpid(), SIGABRT); });

    cleanup_done.wait_for_signal_ready_for(timeout);
}
