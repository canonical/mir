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
#include "mir_test/mock_viewable_area.h"
#include "mir_test/empty_deleter.h"

#include <android/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mis = mi::synthesis;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{geom::Width(1600), geom::Height(1400)}};

class AndroidPointerControllerSetup : public testing::Test
{
public:
    void SetUp()
    {
	controller = std::make_shared<mia::PointerController>(std::shared_ptr<mg::ViewableArea>(&viewable_area, mir::EmptyDeleter()));
    }
protected:
    mg::MockViewableArea viewable_area;
    std::shared_ptr<mia::PointerController> controller;
};

TEST_F(AndroidPointerControllerSetup, button_state_is_saved)
{
    using namespace ::testing;
    
    controller->setButtonState(AKEY_STATE_DOWN);
    EXPECT_EQ(controller->getButtonState(), AKEY_STATE_DOWN);
}

TEST_F(AndroidPointerControllerSetup, position_is_saved)
{
    using namespace ::testing;

    EXPECT_CALL(viewable_area, view_area()).WillOnce(Return(default_view_area));

    controller->setPosition(100,200);

    float out_x, out_y;
    controller->getPosition(&out_x, &out_y);

    EXPECT_EQ(out_x, 100);
    EXPECT_EQ(out_y, 200);
}

TEST_F(AndroidPointerControllerSetup, move_updates_position)
{
    using namespace ::testing;

    EXPECT_CALL(viewable_area, view_area()).Times(2).WillRepeatedly(Return(default_view_area));
    
    controller->setPosition(100, 100);
    controller->move(100, 50);

    float out_x, out_y;
    controller->getPosition(&out_x, &out_y);
    
    EXPECT_EQ(out_x, 200);
    EXPECT_EQ(out_y, 150);
}

TEST_F(AndroidPointerControllerSetup, returns_bounds_of_view_area)
{
    using namespace ::testing;
    float bound_min_x = default_view_area.top_left.x.as_float();
    float bound_min_y = default_view_area.top_left.x.as_float();
    float bound_max_x = default_view_area.size.width.as_float();
    float bound_max_y = default_view_area.size.height.as_float();

    EXPECT_CALL(viewable_area, view_area()).WillOnce(Return(default_view_area));
    
    float out_min_x, out_min_y, out_max_x, out_max_y;
    controller->getBounds(&out_min_x, &out_min_y, &out_max_x, &out_max_y);


    EXPECT_EQ(out_min_x, bound_min_x);
    EXPECT_EQ(out_min_y, bound_min_y);
    EXPECT_EQ(out_max_x, bound_max_x);
    EXPECT_EQ(out_max_y, bound_max_y);
}

TEST_F(AndroidPointerControllerSetup, clips_to_view_area)
{
    using namespace ::testing;

    float bound_min_x = default_view_area.top_left.x.as_float();
    float bound_min_y = default_view_area.top_left.x.as_float();
    float bound_max_x = default_view_area.size.width.as_float();
    float bound_max_y = default_view_area.size.height.as_float();
    float out_x, out_y;

    EXPECT_CALL(viewable_area, view_area()).Times(4).WillRepeatedly(Return(default_view_area));

    controller->setPosition(bound_min_x - 1, 0);
    controller->getPosition(&out_x, &out_y);
    EXPECT_EQ(out_x, bound_min_x);

    controller->setPosition(0,bound_min_y - 1);
    controller->getPosition(&out_x, &out_y);
    EXPECT_EQ(out_y, bound_min_y);
    
    controller->setPosition(bound_max_x + 1.0, 0);
    controller->getPosition(&out_x, &out_y);
    EXPECT_EQ(out_x, bound_max_x);
    
    controller->setPosition(0, bound_max_y + 1);
    controller->getPosition(&out_x, &out_y);
    EXPECT_EQ(out_y, bound_max_y);
}
