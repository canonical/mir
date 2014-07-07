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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/default_emergency_cleanup.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{

struct MockHandler
{
    MOCK_METHOD0(call, void());
};

}

TEST(DefaultEmergencyCleanupTest, works_without_any_handlers)
{
    mir::DefaultEmergencyCleanup cleanup;

    cleanup();
}

TEST(DefaultEmergencyCleanupTest, calls_single_handler)
{
    mir::DefaultEmergencyCleanup cleanup;
    MockHandler handler;

    cleanup.add([&handler] { handler.call(); });

    EXPECT_CALL(handler, call());

    cleanup();
}

TEST(DefaultEmergencyCleanupTest, calls_multiple_handlers_in_order)
{
    mir::DefaultEmergencyCleanup cleanup;
    MockHandler first_handler;
    MockHandler second_handler;
    MockHandler third_handler;

    cleanup.add([&first_handler] { first_handler.call(); });
    cleanup.add([&second_handler] { second_handler.call(); });
    cleanup.add([&third_handler] { third_handler.call(); });

    testing::InSequence s;
    EXPECT_CALL(first_handler, call());
    EXPECT_CALL(second_handler, call());
    EXPECT_CALL(third_handler, call());

    cleanup();
}
