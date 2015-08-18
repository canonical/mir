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

#include "src/server/input/android/android_pointer_controller.h"

#include "mir/test/doubles/mock_input_region.h"
#include "mir/test/doubles/stub_touch_visualizer.h"
#include "mir/test/event_factory.h"
#include "mir/test/fake_shared.h"

#include <android/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{1600, 1400}};

namespace
{

class AndroidPointerControllerSetup : public testing::Test
{
public:
    void SetUp()
    {
        controller = std::make_shared<mia::PointerController>(mt::fake_shared(input_region), std::shared_ptr<mi::CursorListener>(),
                                                              std::make_shared<mtd::StubTouchVisualizer>());
    }
protected:
    mtd::MockInputRegion input_region;
    std::shared_ptr<mia::PointerController> controller;
};

}

TEST_F(AndroidPointerControllerSetup, button_state_is_saved)
{
    using namespace ::testing;

    controller->setButtonState(AKEY_STATE_DOWN);
    EXPECT_EQ(controller->getButtonState(), AKEY_STATE_DOWN);
}

TEST_F(AndroidPointerControllerSetup, position_is_saved)
{
    using namespace ::testing;

    static const float stored_x = 100;
    static const float stored_y = 200;
    geom::Point stored{stored_x, stored_y};

    EXPECT_CALL(input_region, confine(stored));

    controller->setPosition(stored_x, stored_y);

    float saved_x, saved_y;
    controller->getPosition(&saved_x, &saved_y);

    EXPECT_EQ(stored_x, saved_x);
    EXPECT_EQ(stored_y, saved_y);
}

TEST_F(AndroidPointerControllerSetup, move_updates_position)
{
    using namespace ::testing;

    static const float start_x = 100;
    static const float start_y = 100;
    static const float dx = 100;
    static const float dy = 50;
    geom::Point start{start_x, start_y};
    geom::Point moved{start_x + dx, start_y + dy};

    EXPECT_CALL(input_region, confine(start));
    EXPECT_CALL(input_region, confine(moved));

    controller->setPosition(start_x, start_y);
    controller->move(dx, dy);

    float final_x, final_y;
    controller->getPosition(&final_x, &final_y);

    EXPECT_EQ(start_x + dx, final_x);
    EXPECT_EQ(start_y + dy, final_y);
}

TEST_F(AndroidPointerControllerSetup, returns_bounds_of_view_area)
{
    using namespace ::testing;
    EXPECT_CALL(input_region, bounding_rectangle())
        .WillOnce(Return(default_view_area));

    float controller_min_x, controller_min_y, controller_max_x, controller_max_y;
    controller->getBounds(&controller_min_x, &controller_min_y,
                          &controller_max_x, &controller_max_y);

    const float area_min_x = default_view_area.top_left.x.as_float();
    const float area_min_y = default_view_area.top_left.x.as_float();
    const float area_max_x = default_view_area.size.width.as_float();
    const float area_max_y = default_view_area.size.height.as_float();

    EXPECT_EQ(controller_min_x, area_min_x);
    EXPECT_EQ(controller_min_y, area_min_y);
    EXPECT_EQ(controller_max_x, area_max_x);
    EXPECT_EQ(controller_max_y, area_max_y);
}
