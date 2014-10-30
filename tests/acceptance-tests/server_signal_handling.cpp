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
struct TerminateSignal : mtf::InterprocessClientServerTest
{
    MOCK_CONST_METHOD1(callback, void(int));

    void SetUp() override
    {
        init_server([&]
           {
            server.set_terminator([&](int signal)
                {
                    callback(signal);
                    server.stop();
                });
           });
    }
};
}

TEST_F(TerminateSignal, handler_is_called_for_SIGTERM)
{
    run_in_server([&]
        {
            EXPECT_CALL(*this, callback(SIGTERM));
            kill(getpid(), SIGTERM);
            wait_for_server_exit();
        });
}

TEST_F(TerminateSignal, handler_is_called_for_SIGINT)
{
    run_in_server([&]
        {
            EXPECT_CALL(*this, callback(SIGINT));
            kill(getpid(), SIGINT);
            wait_for_server_exit();
        });
}

namespace
{
struct AbortSignal : mtf::InterprocessClientServerTest
{
    std::chrono::milliseconds const timeout{200};

    mtf::CrossProcessSync handler1;

    void SetUp() override
    {
        init_server([&]
           {
            server.add_emergency_cleanup([&]
                { handler1.signal_ready(); });
           });
    }
};
}

TEST_F(AbortSignal, handler_is_called_for_SIGABRT)
{
    run_in_server([&]{ kill(getpid(), SIGABRT); });

    handler1.wait_for_signal_ready_for(timeout);
}
