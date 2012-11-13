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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#include "src/input/android/android_pointer_controller.h"
#include "mir_test/event_factory.h"
#include <android/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;

TEST(AndroidPointerController, button_state_is_saved)
{
    using namespace ::testing;
    mia::PointerController controller;
    
    controller.setButtonState(AKEY_STATE_DOWN);
    EXPECT_EQ(controller.getButtonState(), AKEY_STATE_DOWN);
}

TEST(AndroidPointerController, position_is_saved)
{
    using namespace ::testing;
    mia::PointerController controller;

    controller.setPosition(100,200);

    float out_x, out_y;
    controller.getPosition(&out_x, &out_y);

    EXPECT_EQ(out_x, 100);
    EXPECT_EQ(out_y, 200);
}

TEST(AndroidPointerController, move_updates_position)
{
    using namespace ::testing;
    mia::PointerController controller;
    
    controller.setPosition(100, 100);
    controller.move(100, 50);

    float out_x, out_y;
    controller.getPosition(&out_x, &out_y);
    
    EXPECT_EQ(out_x, 200);
    EXPECT_EQ(out_y, 150);
}

TEST(AndroidPointerController, returns_default_bounds)
{
    using namespace ::testing;
    mia::PointerController controller;
    
    float out_min_x, out_min_y, out_max_x, out_max_y;
    controller.getBounds(&out_min_x, &out_min_y, &out_max_x, &out_max_y);
    EXPECT_EQ(out_min_x, 0);
    EXPECT_EQ(out_min_y, 0);
    EXPECT_EQ(out_max_x, 2048);
    EXPECT_EQ(out_max_y, 2048);
}

