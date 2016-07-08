/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
struct ServerStopCallback : mtf::HeadlessTest
{
    ServerStopCallback() { add_to_environment("MIR_SERVER_NO_FILE", ""); }

    MOCK_CONST_METHOD0(callback0, void());
};
}


TEST_F(ServerStopCallback, is_invoked)
{
    server.add_stop_callback([this]{ callback0(); });
    start_server();

    EXPECT_CALL(*this, callback0());
    stop_server();
}