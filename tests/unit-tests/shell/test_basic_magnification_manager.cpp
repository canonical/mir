/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/shell/basic_magnification_manager.h"

#include "mir/events/event_builders.h"
#include "mir/events/event_helpers.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_composite_event_filter.h"
#include "mir/test/doubles/mock_input_scene.h"
#include "mir/test/doubles/mock_screen_shooter.h"
#include "mir/test/doubles/mock_frontend_surface_stack.h"
#include "mir/test/doubles/mock_input_seat.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std::chrono_literals;
namespace mtd = mir::test::doubles;
namespace mev = mir::events;
namespace geom = mir::geometry;

struct TestBasicMagnificationManager : Test
{
    TestBasicMagnificationManager()
        : scene(std::make_shared<NiceMock<mtd::MockInputScene>>()),
          screen_shooter(std::make_shared<NiceMock<mtd::MockScreenShooter>>()),
          stub_composite_event_filter(std::make_shared<mtd::StubCompositeEventFilter>()),
          manager(
            stub_composite_event_filter,
            scene,
            std::make_shared<mtd::StubBufferAllocator>(),
            screen_shooter,
            std::make_shared<NiceMock<mtd::MockFrontendSurfaceStack>>(),
            std::make_shared<NiceMock<mtd::MockInputSeat>>())
    {
    }

    std::shared_ptr<mtd::MockInputScene> scene;
    std::shared_ptr<mtd::MockScreenShooter> screen_shooter;
    std::shared_ptr<mtd::StubCompositeEventFilter> stub_composite_event_filter;
    mir::shell::BasicMagnificationManager manager;
};

TEST_F(TestBasicMagnificationManager, enabling_adds_renderable_to_input_scene)
{
    EXPECT_CALL(*scene, prepend_input_visualization(testing::_));
    EXPECT_THAT(manager.enabled(true), Eq(true));
}

TEST_F(TestBasicMagnificationManager, enabling_twice_returns_false_second_time)
{
    EXPECT_THAT(manager.enabled(true), Eq(true));
    EXPECT_THAT(manager.enabled(true), Eq(false));
}

TEST_F(TestBasicMagnificationManager, enabling_prepends_input_visualization_to_the_scene)
{
    EXPECT_CALL(*scene, prepend_input_visualization(testing::_));
    manager.enabled(true);
}

TEST_F(TestBasicMagnificationManager, disabling_removes_input_visualization_to_the_scene)
{
    EXPECT_CALL(*scene, remove_input_visualization(testing::_));
    manager.enabled(true);
    manager.enabled(false);
}

TEST_F(TestBasicMagnificationManager, disabling_twice_returns_false_second_time)
{
    EXPECT_THAT(manager.enabled(true), Eq(true));
    EXPECT_THAT(manager.enabled(false), Eq(true));
    EXPECT_THAT(manager.enabled(false), Eq(false));
}

TEST_F(TestBasicMagnificationManager, enabling_causes_screen_shooter_to_capture)
{
    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
    ));
    manager.enabled(true);
}

TEST_F(TestBasicMagnificationManager, enabling_removes_renderable_from_input_scene)
{
    EXPECT_CALL(*scene, remove_input_visualization(testing::_));
    EXPECT_THAT(manager.enabled(true), Eq(true));
    EXPECT_THAT(manager.enabled(false), Eq(true));
}

TEST_F(TestBasicMagnificationManager, pointer_movement_event_updates_next_capture)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(100, 100),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
    ));

    auto const ev = mev::make_pointer_event(
        1,
        0s,
        mir_input_event_modifier_none,
        mir_pointer_action_motion,
        0,
        geom::PointF(300, 250),
        geom::DisplacementF(0, 0),
        mir_pointer_axis_source_none,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{});
    stub_composite_event_filter->handle(*ev);
}

TEST_F(TestBasicMagnificationManager, pointer_movement_event_without_position_does_not_trigger_capture)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter).Times(0);

    auto const ev = mev::make_pointer_event(
        1,
        0s,
        mir_input_event_modifier_none,
        mir_pointer_action_motion,
        0,
        std::nullopt,
        geom::DisplacementF(0, 0),
        mir_pointer_axis_source_none,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{});
    stub_composite_event_filter->handle(*ev);
}

TEST_F(TestBasicMagnificationManager, pointer_movement_event_with_non_motion_event_does_not_trigger_capture)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter).Times(0);

    auto const ev = mev::make_pointer_event(
        1,
        0s,
        mir_input_event_modifier_none,
        mir_pointer_action_button_down,
        0,
        geom::PointF(300, 250),
        geom::DisplacementF(0, 0),
        mir_pointer_axis_source_none,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{});
    stub_composite_event_filter->handle(*ev);
}

TEST_F(TestBasicMagnificationManager, non_pointer_event_does_not_trigger_capture)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter).Times(0);

    auto const ev = mev::make_key_event(
        1,
        0s,
        mir_keyboard_action_down,
        XKB_KEY_A,
        0,
        mir_input_event_modifier_none);
    stub_composite_event_filter->handle(*ev);
}

TEST_F(TestBasicMagnificationManager, non_input_event_does_not_trigger_capture)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter).Times(0);

    mir::frontend::SurfaceId id;
    auto const ev = mev::make_window_resize_event(
        id,
        geom::Size(400, 300));
    stub_composite_event_filter->handle(*ev);
}

TEST_F(TestBasicMagnificationManager, setting_size_updates_with_new_size)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-100, -50),
            mir::geometry::Size(200, 100)),
        testing::_,
        false,
        testing::_
    ));

    manager.size(geom::Size(200, 100));
    EXPECT_THAT(manager.size(), Eq(geom::Size(200, 100)));
}

TEST_F(TestBasicMagnificationManager, setting_magnification_updates_with_old_size)
{
    InSequence seq;

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
        )).WillOnce(WithArg<4>(Invoke([&](std::function<void(std::optional<mir::time::Timestamp>)>&& cb)
            {
                cb(std::chrono::steady_clock::now());
            })));
    manager.enabled(true);

    EXPECT_CALL(*screen_shooter, capture_with_filter(
        testing::_,
        mir::geometry::Rectangle(
            mir::geometry::Point(-200, -150),
            mir::geometry::Size(400, 300)),
        testing::_,
        false,
        testing::_
    ));

    manager.magnification(4.f);
    EXPECT_THAT(manager.magnification(), Eq(4.f));
}
