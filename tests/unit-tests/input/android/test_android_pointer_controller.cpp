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

#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test/event_factory.h"
#include "mir_test/fake_shared.h"

#include <android/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{geom::Width(1600),
                                                                            geom::Height(1400)}};

namespace
{

class AndroidPointerControllerSetup : public testing::Test
{
public:
    void SetUp()
    {
        controller = std::make_shared<mia::PointerController>(mt::fake_shared(viewable_area));
    }
protected:
    mtd::MockViewableArea viewable_area;
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

    EXPECT_CALL(viewable_area, view_area()).WillOnce(Return(default_view_area));

    controller->setPosition(stored_x, stored_y);

    float saved_x, saved_y;
    controller->getPosition(&saved_x, &saved_y);

    EXPECT_EQ(stored_x, saved_x);
    EXPECT_EQ(stored_y, saved_y);
}

TEST_F(AndroidPointerControllerSetup, move_updates_position)
{
    using namespace ::testing;

    EXPECT_CALL(viewable_area, view_area()).Times(2)
        .WillRepeatedly(Return(default_view_area));

    static const float start_x = 100;
    static const float start_y = 100;
    static const float dx = 100;
    static const float dy = 50;

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
    EXPECT_CALL(viewable_area, view_area()).WillOnce(Return(default_view_area));

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

TEST_F(AndroidPointerControllerSetup, clips_to_view_area)
{
    using namespace ::testing;

    float min_x_bound = default_view_area.top_left.x.as_float();
    float min_y_bound = default_view_area.top_left.x.as_float();
    float max_x_bound = min_x_bound + default_view_area.size.width.as_float();
    float max_y_bound = min_y_bound + default_view_area.size.height.as_float();

    static const float invalid_lower_bound_x = min_x_bound - 1;
    static const float invalid_lower_bound_y = min_y_bound - 1;
    static const float invalid_upper_bound_x = max_x_bound + 1;
    static const float invalid_upper_bound_y = max_y_bound + 1;

    EXPECT_CALL(viewable_area, view_area()).Times(4).WillRepeatedly(Return(default_view_area));

    float bounded_x, bounded_y;

    controller->setPosition(invalid_lower_bound_x, 0);
    controller->getPosition(&bounded_x, &bounded_y);
    EXPECT_EQ(min_x_bound, bounded_x);

    controller->setPosition(0, invalid_lower_bound_y);
    controller->getPosition(&bounded_x, &bounded_y);
    EXPECT_EQ(min_y_bound, bounded_y);

    controller->setPosition(invalid_upper_bound_x, 0);
    controller->getPosition(&bounded_x, &bounded_y);
    EXPECT_EQ(max_x_bound, bounded_x);

    controller->setPosition(0, invalid_upper_bound_y);
    controller->getPosition(&bounded_x, &bounded_y);
    EXPECT_EQ(max_y_bound, bounded_y);
}
