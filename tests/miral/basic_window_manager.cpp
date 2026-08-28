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

#include "test_window_manager_tools.h"

namespace mt = mir::test;

using namespace testing;

struct BasicWindowManager : mt::TestWindowManagerTools
{
};

TEST_F(BasicWindowManager, window_at_returns_empty_for_unmanaged_surface)
{
    ON_CALL(focus_controller, surface_at(_))
        .WillByDefault([&](auto) { return create_surface(session, {}); });

    EXPECT_EQ(basic_window_manager.window_at({}), miral::Window{});
}
