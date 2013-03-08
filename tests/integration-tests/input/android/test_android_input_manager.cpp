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
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "mir/input/event_filter.h"
#include "src/input/android/default_android_input_configuration.h"
#include "src/input/android/android_input_manager.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"

#include <EventHub.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using mtd::MockEventFilter;
using mir::WaitCondition;

namespace
{
using namespace ::testing;

static const geom::Rectangle default_view_area =
        geom::Rectangle{geom::Point(),
                        geom::Size{geom::Width(1600), geom::Height(1400)}};

static const std::shared_ptr<mi::CursorListener> null_cursor_listener{};

class AndroidInputManagerAndEventFilterDispatcherSetup : public testing::Test
{
public:
    AndroidInputManagerAndEventFilterDispatcherSetup()
    {
        configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(std::initializer_list<std::shared_ptr<mi::EventFilter> const>{mt::fake_shared(event_filter)}, mt::fake_shared(viewable_area), null_cursor_listener);
        ON_CALL(viewable_area, view_area())
            .WillByDefault(Return(default_view_area));
        
        fake_event_hub = configuration->the_fake_event_hub();

        input_manager = std::make_shared<mia::InputManager>(configuration);

        input_manager->start();
    }

    void TearDown()
    {
        input_manager->stop();
    }

  protected:
    std::shared_ptr<mtd::FakeEventHubInputConfiguration> configuration;
    mia::FakeEventHub* fake_event_hub;
    std::shared_ptr<mia::InputManager> input_manager;
    MockEventFilter event_filter;
    NiceMock<mtd::MockViewableArea> viewable_area;
};

}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_key_events_to_filter)
{
    using namespace ::testing;

    WaitCondition wait_condition;

    EXPECT_CALL(
        event_filter,
        handles(KeyDownEvent()))
            .Times(1)
            .WillOnce(ReturnFalseAndWakeUp(&wait_condition));

    fake_event_hub->synthesize_builtin_keyboard_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_ENTER));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_button_events_to_filter)
{
    using namespace ::testing;

    WaitCondition wait_condition;

    EXPECT_CALL(
        event_filter,
        handles(ButtonDownEvent()))
            .Times(1)
            .WillOnce(ReturnFalseAndWakeUp(&wait_condition));

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_motion_events_to_filter)
{
    using namespace ::testing;

    WaitCondition wait_condition;

    // We get absolute motion events since we have a pointer controller.
    {
        InSequence seq;

        EXPECT_CALL(event_filter,
                    handles(MotionEvent(100, 100)))
            .WillOnce(Return(false));
        EXPECT_CALL(event_filter,
                    handles(MotionEvent(200, 100)))
            .WillOnce(ReturnFalseAndWakeUp(&wait_condition));
    }

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(100,100));
    fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(100,0));

    wait_condition.wait_for_at_most_seconds(1);
}

