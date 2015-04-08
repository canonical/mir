/*
 * Copyright © 2015 Canonical Ltd.
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

#include "src/server/input/default_input_dispatcher.h"

#include "mir/events/event_builders.h"
#include "mir/events/event_private.h"
#include "mir/scene/observer.h"

#include "mir_test/event_matchers.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_input_scene.h"
#include "mir_test_doubles/mock_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mutex>
#include <algorithm>

namespace ms = mir::scene;
namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace geom = mir::geometry;

namespace
{

struct MockSurfaceWithGeometry : public mtd::MockSurface
{
    MockSurfaceWithGeometry(geom::Rectangle const& geom)
        : geom(geom)
    {
    }

    bool input_area_contains(geom::Point const& p) const override
    {
        return geom.contains(p);
    }

    geom::Rectangle input_bounds() const override
    {
        return geom;
    }
    
    geom::Rectangle const geom;
};

struct StubInputScene : public mtd::StubInputScene
{
    std::shared_ptr<mtd::MockSurface> add_surface(geom::Rectangle const& geometry)
    {
        std::lock_guard<std::mutex> lg(surface_guard);
        
        auto surface = std::make_shared<MockSurfaceWithGeometry>(geometry);
        surfaces.push_back(surface);

        observer->surface_added(surface.get());
        
        return surface;
    }

    void remove_surface(std::shared_ptr<mi::Surface> const& surface)
    {
        std::lock_guard<std::mutex> lg(surface_guard);
        auto it = std::find(surfaces.begin(), surfaces.end(), surface);
        assert(it != surfaces.end());
        auto surf = *it;
        surfaces.erase(it);
        observer->surface_removed(surf.get());
    }

    std::shared_ptr<mtd::MockSurface> add_surface()
    {
        return add_surface({{0, 0}, {1, 1}});
    }
    
    void for_each(std::function<void(std::shared_ptr<mi::Surface> const&)> const& exec) override
    {
        std::lock_guard<std::mutex> lg(surface_guard);
        for (auto const& surface : surfaces)
            exec(surface);
    }

    void add_observer(std::shared_ptr<ms::Observer> const& new_observer) override
    {
        std::lock_guard<std::mutex> lg(surface_guard);
        assert(observer == nullptr);
        observer = new_observer;
        
        for (auto const& surface : surfaces)
        {
            observer->surface_exists(surface.get());
        }
    }
    
    void remove_observer(std::weak_ptr<ms::Observer> const& /* remove_observer */) override
    {
        assert(observer != nullptr);
        observer->end_observation();
        observer.reset();
    }
    
    std::mutex surface_guard;
    std::vector<std::shared_ptr<ms::Surface>> surfaces;

    std::shared_ptr<ms::Observer> observer;
};

struct DefaultInputDispatcher : public testing::Test
{
    DefaultInputDispatcher()
        : dispatcher(mt::fake_shared(scene))
    {
    }

    void TearDown() override { dispatcher.stop(); }

    mi::DefaultInputDispatcher dispatcher;
    StubInputScene scene;
};

mir::EventUPtr a_key_event(MirKeyboardAction action = mir_keyboard_action_down)
{
    // TODO: More
    return mev::make_event(0, 0, action, 0, 0, mir_input_event_modifier_alt);
}

// TODO: Impl fake device ID
struct FakePointer
{
    mir::EventUPtr move_to(geom::Point const& location)
    {
        return mev::make_event(0, 0, 0, mir_pointer_action_motion, buttons_pressed,
                               location.x.as_int(), location.y.as_int(),
                               0, 0);
    }
    mir::EventUPtr release_button(geom::Point const& location, MirPointerButton button = mir_pointer_button_primary)
    {
        std::remove_if(buttons_pressed.begin(), buttons_pressed.end(), [&button](MirPointerButton b){
            return b == button;
        });

        return mev::make_event(0, 0, 0, mir_pointer_action_button_up, buttons_pressed,
                               location.x.as_int(), location.y.as_int(),
                               0, 0);
    }
    mir::EventUPtr press_button(geom::Point const& location, MirPointerButton button = mir_pointer_button_primary)
    {
        buttons_pressed.push_back(button);
        return mev::make_event(0, 0, 0, mir_pointer_action_button_down, buttons_pressed,
                               location.x.as_int(), location.y.as_int(),
                               0, 0);
    }

    std::vector<MirPointerButton> buttons_pressed;
};

// TODO: Multi-touch
// TODO: Device IDs
struct FakeToucher
{
    mir::EventUPtr move_to(geom::Point const& point)
    {
        auto ev = mev::make_event(0, 0, 0);
        mev::add_touch(*ev, 0, mir_touch_action_change,
                       mir_touch_tooltype_finger, point.x.as_int(), point.y.as_int(),
                       touched ? 1.0 : 0.0,
                       touched ? 1.0 : 0.0,
                       touched ? 1.0 : 0.0,
                       touched ? 1.0 : 0.0);
        return ev;
    }
    mir::EventUPtr touch_at(geom::Point const& point)
    {
        touched = true;
        
        auto ev = mev::make_event(0, 0, 0);
        mev::add_touch(*ev, 0, mir_touch_action_down,
                       mir_touch_tooltype_finger, point.x.as_int(), point.y.as_int(),
                       1.0, 1.0, 1.0, 1.0);
        return ev;
    }
    mir::EventUPtr release_at(geom::Point const& point)
    {
        touched = false;
        
        auto ev = mev::make_event(0, 0, 0);
        mev::add_touch(*ev, 0, mir_touch_action_up,
                       mir_touch_tooltype_finger, point.x.as_int(), point.y.as_int(),
                       0.0, 0.0, 0.0, 0.0);
        return ev;
    }
    bool touched = false;
};

}

TEST_F(DefaultInputDispatcher, key_event_delivered_to_focused_surface)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface();

    auto event = a_key_event();

    EXPECT_CALL(*surface, consume(mt::MirKeyEventMatches(*event))).Times(1);

    dispatcher.start();

    dispatcher.set_focus(surface);
    EXPECT_TRUE(dispatcher.dispatch(*event));
}

TEST_F(DefaultInputDispatcher, key_event_dropped_if_no_surface_focused)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface();
    auto event = a_key_event();
    
    EXPECT_CALL(*surface, consume(_)).Times(0);

    dispatcher.start();
    EXPECT_FALSE(dispatcher.dispatch(*event));
}

TEST_F(DefaultInputDispatcher, inconsistent_key_events_dropped)
{
    using namespace ::testing;

    auto surface = scene.add_surface();
    auto event = a_key_event(mir_keyboard_action_up);

    EXPECT_CALL(*surface, consume(mt::MirKeyEventMatches(*event))).Times(0);

    dispatcher.start();

    dispatcher.set_focus(surface);
    
    EXPECT_FALSE(dispatcher.dispatch(*event));
}

TEST_F(DefaultInputDispatcher, key_state_is_consistent_per_client)
{
    using namespace ::testing;

    auto surface_1 = scene.add_surface();
    auto surface_2 = scene.add_surface();

    auto down_event = a_key_event();
    auto up_event= a_key_event(mir_keyboard_action_up);

    EXPECT_CALL(*surface_1, consume(mt::MirKeyEventMatches(*down_event))).Times(1);
    EXPECT_CALL(*surface_2, consume(_)).Times(0);

    dispatcher.start();

    dispatcher.set_focus(surface_1);
    EXPECT_TRUE(dispatcher.dispatch(*down_event));
    dispatcher.set_focus(surface_2);
    EXPECT_TRUE(dispatcher.dispatch(*up_event));
}
// TODO: Audit key state tests but this seems like a good start...

// TODO: Test that pointer motion is relative
TEST_F(DefaultInputDispatcher, pointer_motion_delivered_to_client_under_pointer)
{
    using namespace ::testing;

    auto surface = scene.add_surface({{0, 0}, {5, 5}});

    FakePointer pointer;
    auto ev_1 = pointer.move_to({1, 0});
    auto ev_2 = pointer.move_to({5, 0});

    InSequence seq;
    // The cursor begins at 0, 0
    EXPECT_CALL(*surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*surface, consume(mt::PointerEventWithPosition(1, 0))).Times(1);
    // TODO: Test coordinate on PointerExit and PointerEnter event
    EXPECT_CALL(*surface, consume(mt::PointerLeaveEvent())).Times(1);

    dispatcher.start();

    EXPECT_TRUE(dispatcher.dispatch(*pointer.move_to({1, 0})));
    EXPECT_TRUE(dispatcher.dispatch(*pointer.move_to({5, 0})));
}

TEST_F(DefaultInputDispatcher, pointer_delivered_only_to_top_surface)
{
    using namespace ::testing;

    auto surface = scene.add_surface({{0, 0}, {5, 5}});
    auto top_surface = scene.add_surface({{0, 0}, {5, 5}});

    FakePointer pointer;

    InSequence seq;
    // The cursor begins at 0, 0
    EXPECT_CALL(*top_surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*top_surface, consume(mt::PointerEventWithPosition(1, 0))).Times(1);
    EXPECT_CALL(*surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*surface, consume(mt::PointerEventWithPosition(1, 0))).Times(1);
    
    dispatcher.start();

    EXPECT_TRUE(dispatcher.dispatch(*pointer.move_to({1, 0})));
    scene.remove_surface(top_surface);
    EXPECT_TRUE(dispatcher.dispatch(*pointer.move_to({1, 0})));
}

TEST_F(DefaultInputDispatcher, pointer_may_move_between_adjacent_surfaces)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface({{0, 0}, {5, 5}});
    auto another_surface = scene.add_surface({{5, 5}, {5, 5}});

    FakePointer pointer;
    auto ev_1 = pointer.move_to({6, 6});
    auto ev_2 = pointer.move_to({7, 7});

    InSequence seq;
    EXPECT_CALL(*surface, consume(mt::PointerEnterEvent())).Times(1);
    // TODO: Position on leave event
    EXPECT_CALL(*surface, consume(mt::PointerLeaveEvent())).Times(1);
    EXPECT_CALL(*another_surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*another_surface, consume(mt::PointerEventWithPosition(1, 0))).Times(1);
    EXPECT_CALL(*another_surface, consume(mt::PointerLeaveEvent())).Times(1);
    
    dispatcher.start();

    EXPECT_TRUE(dispatcher.dispatch(*ev_1));
    EXPECT_TRUE(dispatcher.dispatch(*ev_2));
}

// We test that a client will receive pointer events following a button down
// until the pointer comes up.
// TODO: What happens when a client dissapears during a gesture?
TEST_F(DefaultInputDispatcher, gestures_persist_over_button_down)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface({{0, 0}, {5, 5}});
    auto another_surface = scene.add_surface({{5, 5}, {5, 5}});

    FakePointer pointer;
    auto ev_1 = pointer.press_button({0, 0});
    auto ev_2 = pointer.move_to({6, 6});
    auto ev_3 = pointer.release_button({6, 6});

    InSequence seq;
    EXPECT_CALL(*surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*surface, consume(mt::ButtonDownEvent(0,0))).Times(1);
    // TODO: Position on leave event
    EXPECT_CALL(*surface, consume(mt::PointerEventWithPosition(6, 6))).Times(1);
    EXPECT_CALL(*surface, consume(mt::ButtonUpEvent(6,6))).Times(1);
    EXPECT_CALL(*surface, consume(mt::PointerLeaveEvent())).Times(1);
    EXPECT_CALL(*another_surface, consume(mt::PointerEnterEvent())).Times(1);
    
    dispatcher.start();

    EXPECT_TRUE(dispatcher.dispatch(*ev_1));
    EXPECT_TRUE(dispatcher.dispatch(*ev_2));
    EXPECT_TRUE(dispatcher.dispatch(*ev_3));
}

TEST_F(DefaultInputDispatcher, pointer_gestures_may_transfer_over_buttons)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface({{0, 0}, {5, 5}});
    auto another_surface = scene.add_surface({{5, 5}, {5, 5}});

    FakePointer pointer;
    auto ev_1 = pointer.press_button({0, 0}, mir_pointer_button_primary);
    auto ev_2 = pointer.press_button({0, 0}, mir_pointer_button_secondary);
    auto ev_3 = pointer.release_button({0, 0}, mir_pointer_button_primary);
    auto ev_4 = pointer.move_to({6, 6});
    auto ev_5 = pointer.release_button({6, 6}, mir_pointer_button_secondary);

    InSequence seq;
    EXPECT_CALL(*surface, consume(mt::PointerEnterEvent())).Times(1);
    EXPECT_CALL(*surface, consume(mt::ButtonDownEvent(0,0))).Times(1);
    EXPECT_CALL(*surface, consume(mt::ButtonDownEvent(0,0))).Times(1);
    EXPECT_CALL(*surface, consume(mt::ButtonUpEvent(0,0))).Times(1);
    // TODO: Position on leave event
    EXPECT_CALL(*surface, consume(mt::PointerEventWithPosition(6, 6))).Times(1);
    EXPECT_CALL(*surface, consume(mt::PointerLeaveEvent())).Times(1);
    EXPECT_CALL(*another_surface, consume(mt::PointerEnterEvent())).Times(1);

    dispatcher.start();

    EXPECT_TRUE(dispatcher.dispatch(*ev_1));
    EXPECT_TRUE(dispatcher.dispatch(*ev_2));
    EXPECT_TRUE(dispatcher.dispatch(*ev_3));
    EXPECT_TRUE(dispatcher.dispatch(*ev_4));
    EXPECT_TRUE(dispatcher.dispatch(*ev_5));
}
// TODO: Test for what happens to gesture when client dissapears

// TODO: Add touch event builders
TEST_F(DefaultInputDispatcher, touch_delivered_to_surface)
{
    using namespace ::testing;
    
    auto surface = scene.add_surface({{1, 1}, {1, 1}});

    InSequence seq;
    EXPECT_CALL(*surface, consume(mt::TouchEvent(0,0))).Times(1);
    EXPECT_CALL(*surface, consume(mt::TouchUpEvent(0,0))).Times(1);

    dispatcher.start();

    FakeToucher toucher;
    EXPECT_TRUE(dispatcher.dispatch(*toucher.touch_at({1,1})));
    EXPECT_TRUE(dispatcher.dispatch(*toucher.release_at({1,1})));
}

TEST_F(DefaultInputDispatcher, touch_delivered_only_to_top_surface)
{
    using namespace ::testing;
    
    auto bottom_surface = scene.add_surface({{1, 1}, {3, 3}});
    auto surface = scene.add_surface({{1, 1}, {3, 3}});

    InSequence seq;
    EXPECT_CALL(*surface, consume(mt::TouchEvent(0,0))).Times(1);
    EXPECT_CALL(*surface, consume(mt::TouchUpEvent(0,0))).Times(1);
    EXPECT_CALL(*bottom_surface, consume(mt::TouchEvent(0,0))).Times(0);
    EXPECT_CALL(*bottom_surface, consume(mt::TouchUpEvent(0,0))).Times(0);

    dispatcher.start();

    FakeToucher toucher;
    EXPECT_TRUE(dispatcher.dispatch(*toucher.move_to({1,1})));
    EXPECT_TRUE(dispatcher.dispatch(*toucher.move_to({2,2})));
}

// TODO: Test that touch can move between surfaces beside eachother

TEST_F(DefaultInputDispatcher, gestures_persist_over_touch_down)
{
    using namespace ::testing;
    
    auto left_surface = scene.add_surface({{0, 0}, {1, 1}});
    auto right_surface = scene.add_surface({{1, 1}, {1, 1}});

    InSequence seq;
    EXPECT_CALL(*left_surface, consume(mt::TouchEvent(0, 0))).Times(1);
    // Mathc movement pos?
    EXPECT_CALL(*left_surface, consume(mt::TouchMovementEvent())).Times(1);
    EXPECT_CALL(*left_surface, consume(mt::TouchUpEvent(2, 2))).Times(1);
    EXPECT_CALL(*right_surface, consume(_)).Times(0);

    dispatcher.start();
    
    FakeToucher toucher;
    EXPECT_TRUE(dispatcher.dispatch(*toucher.touch_at({0, 0})));
    EXPECT_TRUE(dispatcher.dispatch(*toucher.move_to({2, 2})));
    EXPECT_TRUE(dispatcher.dispatch(*toucher.release_at({2, 2})));
}

// TODO: Test multiple touch gesture

// TODO: Test that gesture tracking is per device
