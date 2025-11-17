/*
 * Copyright © Canonical Ltd.
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
    MOCK_METHOD(void, stop_callback0, (), (const, override));
    MOCK_METHOD(void, stop_callback1, (), (const, override));
    MOCK_METHOD(void, stop_callback2, (), (const, override));
};
}

TEST_F(ServerStopCallback, is_invoked)
{
    server.add_stop_callback([this]{ stop_callback0(); });

    start_server();

    EXPECT_CALL(*this, stop_callback0());
    stop_server();
}

TEST_F(ServerStopCallback, invoked_in_reverse_order)
{
    server.add_stop_callback([this]{ stop_callback0(); });
    server.add_stop_callback([this]{ stop_callback1(); });
    server.add_stop_callback([this]{ stop_callback2(); });

    start_server();

    testing::InSequence seq;
    EXPECT_CALL(*this, stop_callback2());
    EXPECT_CALL(*this, stop_callback1());
    EXPECT_CALL(*this, stop_callback0());

    stop_server();
}
