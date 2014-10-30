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

#include "mir_test_framework/headless_test.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <csignal>

namespace mtf = mir_test_framework;
namespace
{
struct TerminateSignal : mtf::HeadlessTest
{
    MOCK_CONST_METHOD1(callback, void(int));

    void SetUp() override
    {
        server.set_terminator([&](int signal)
            {
                callback(signal);
                server.stop();
            });

        start_server();
    }

    void TearDown() override
    {
        wait_for_server_exit();
    }
};
}

TEST_F(TerminateSignal, handler_is_called_for_SIGTERM)
{
    EXPECT_CALL(*this, callback(SIGTERM));
    kill(getpid(), SIGTERM);
}

TEST_F(TerminateSignal, handler_is_called_for_SIGINT)
{
    EXPECT_CALL(*this, callback(SIGINT));
    kill(getpid(), SIGINT);
}
