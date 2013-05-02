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
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/input/android/android_input_configuration.h"

#include "src/server/input/android/default_android_input_configuration.h"
#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_dispatcher_controller.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test_doubles/stub_session_target.h"
#include "mir_test_doubles/stub_surface_target.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"
#include "mir_test/event_matchers.h"

#include <EventHub.h>
#include <InputDispatcher.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/socket.h>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using mtd::MockEventFilter;

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
        event_filter = std::make_shared<MockEventFilter>();
        configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(std::initializer_list<std::shared_ptr<mi::EventFilter> const>{event_filter}, mt::fake_shared(viewable_area), null_cursor_listener);
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
    std::shared_ptr<MockEventFilter> event_filter;
    NiceMock<mtd::MockViewableArea> viewable_area;
};

}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_key_events_to_filter)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    EXPECT_CALL(
        *event_filter,
        handles(mt::KeyDownEvent()))
            .Times(1)
            .WillOnce(mt::ReturnFalseAndWakeUp(&wait_condition));

    fake_event_hub->synthesize_builtin_keyboard_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_ENTER));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_button_down_events_to_filter)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    EXPECT_CALL(
        *event_filter,
        handles(mt::ButtonDownEvent()))
            .Times(1)
            .WillOnce(mt::ReturnFalseAndWakeUp(&wait_condition));

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_button_up_events_to_filter)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    {
      InSequence seq;
      EXPECT_CALL(
          *event_filter,
          handles(mt::ButtonDownEvent()))
              .Times(1)
              .WillOnce(Return(false));
      EXPECT_CALL(
          *event_filter,
          handles(mt::ButtonUpEvent()))
              .Times(1)
              .WillOnce(Return(false));
      EXPECT_CALL(
          *event_filter,
          handles(mt::MotionEvent(0,0)))
              .Times(1)
              .WillOnce(mt::ReturnFalseAndWakeUp(&wait_condition));
    }

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_button_down_event().of_button(BTN_LEFT));
    fake_event_hub->synthesize_event(mis::a_button_up_event().of_button(BTN_LEFT));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_motion_events_to_filter)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    // We get absolute motion events since we have a pointer controller.
    {
        InSequence seq;

        EXPECT_CALL(*event_filter,
                    handles(mt::MotionEvent(100, 100)))
            .WillOnce(Return(false));
        EXPECT_CALL(*event_filter,
                    handles(mt::MotionEvent(200, 100)))
            .WillOnce(mt::ReturnFalseAndWakeUp(&wait_condition));
    }

    fake_event_hub->synthesize_builtin_cursor_added();
    fake_event_hub->synthesize_device_scan_complete();

    fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(100, 100));
    fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(100, 0));

    wait_condition.wait_for_at_most_seconds(1);
}

namespace
{

struct MockDispatcherPolicy : public mia::EventFilterDispatcherPolicy
{
    MockDispatcherPolicy(std::shared_ptr<mi::EventFilter> const& filter)
      : EventFilterDispatcherPolicy(filter, false)
    {
    }
    MOCK_METHOD3(interceptKeyBeforeDispatching, nsecs_t(droidinput::sp<droidinput::InputWindowHandle> const&,
                                                        droidinput::KeyEvent const*, uint32_t));
};

struct TestingInputConfiguration : public mtd::FakeEventHubInputConfiguration
{
    TestingInputConfiguration(std::shared_ptr<mi::EventFilter> const& filter,
                              std::shared_ptr<mg::ViewableArea> const& view_area,
                              std::shared_ptr<mi::CursorListener> const& cursor_listener)
        : FakeEventHubInputConfiguration({}, view_area, cursor_listener),
          dispatcher_policy(new MockDispatcherPolicy(filter))
    {
    }
    droidinput::sp<droidinput::InputDispatcherPolicyInterface> the_dispatcher_policy()
    {
        return dispatcher_policy;
    }
    droidinput::sp<MockDispatcherPolicy> the_mock_dispatcher_policy()
    {
        return dispatcher_policy;
    }

    droidinput::sp<MockDispatcherPolicy> dispatcher_policy;
};

struct AndroidInputManagerDispatcherInterceptSetup : public testing::Test
{
    AndroidInputManagerDispatcherInterceptSetup()
    {
        event_filter = std::make_shared<MockEventFilter>();
        configuration = std::make_shared<TestingInputConfiguration>(
            event_filter,
            mt::fake_shared(viewable_area), null_cursor_listener);
        fake_event_hub = configuration->the_fake_event_hub();

        ON_CALL(viewable_area, view_area())
            .WillByDefault(Return(default_view_area));
        input_manager = std::make_shared<mia::InputManager>(configuration);
        input_target_listener = std::make_shared<mia::DispatcherController>(configuration);

        dispatcher_policy = configuration->the_mock_dispatcher_policy();

    }

    ~AndroidInputManagerDispatcherInterceptSetup()
    {
        input_manager->stop();
    }

    // TODO: It would be nice if it were possible to mock the interface between
    // droidinput::InputChannel and droidinput::InputDispatcher rather than use
    // valid fds to allow non-throwing construction of a real input channel.
    void SetUp()
    {
        input_manager->start();
    }
    
    int test_fd()
    {
        int fds[2];
        // Closed by droidinput InputChannel on shutdown
        socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);
        return fds[0];
    }

    std::shared_ptr<MockEventFilter> event_filter;
    NiceMock<mtd::MockViewableArea> viewable_area;
    std::shared_ptr<TestingInputConfiguration> configuration;
    mia::FakeEventHub* fake_event_hub;
    droidinput::sp<MockDispatcherPolicy> dispatcher_policy;

    std::shared_ptr<mia::InputManager> input_manager;
    std::shared_ptr<msh::InputTargetListener> input_target_listener;
};

MATCHER_P(WindowHandleWithInputFd, input_fd, "")
{
    if (arg->getInputChannel()->getFd() == input_fd)
        return true;
    return false;
}

}

TEST_F(AndroidInputManagerDispatcherInterceptSetup, server_input_fd_of_focused_surface_is_sent_unfiltered_key_events)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    mtd::StubSessionTarget session;
    
    auto input_fd = test_fd();
    mtd::StubSurfaceTarget surface(input_fd);

    EXPECT_CALL(*event_filter, handles(_)).Times(1).WillOnce(Return(false));
    // We return -1 here to skip publishing of the event (to an unconnected test socket!).
    EXPECT_CALL(*dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(input_fd), _, _))
        .Times(1).WillOnce(DoAll(mt::WakeUp(&wait_condition), Return(-1)));

    input_target_listener->input_application_opened(mt::fake_shared(session));
    input_target_listener->input_surface_opened(mt::fake_shared(session), mt::fake_shared(surface));
    input_target_listener->focus_changed(mt::fake_shared(surface));

    fake_event_hub->synthesize_builtin_keyboard_added();
    fake_event_hub->synthesize_device_scan_complete();
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_ENTER));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerDispatcherInterceptSetup, changing_focus_changes_event_recipient)
{
    using namespace ::testing;

    mt::WaitCondition wait1, wait2, wait3;

    mtd::StubSessionTarget session;
    
    auto input_fd_1 = test_fd();
    mtd::StubSurfaceTarget surface1(input_fd_1);
    auto input_fd_2 = test_fd();
    mtd::StubSurfaceTarget surface2(input_fd_2);

    input_target_listener->input_application_opened(mt::fake_shared(session));
    input_target_listener->input_surface_opened(mt::fake_shared(session), mt::fake_shared(surface1));
    input_target_listener->input_surface_opened(mt::fake_shared(session), mt::fake_shared(surface2));

    EXPECT_CALL(*event_filter, handles(_)).Times(3).WillRepeatedly(Return(false));

    {
        InSequence seq;

        EXPECT_CALL(*dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(input_fd_1), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait1), Return(-1)));
        EXPECT_CALL(*dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(input_fd_2), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait2), Return(-1)));
        EXPECT_CALL(*dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(input_fd_1), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait3), Return(-1)));
    }

    fake_event_hub->synthesize_builtin_keyboard_added();
    fake_event_hub->synthesize_device_scan_complete();

    input_target_listener->focus_changed(mt::fake_shared(surface1));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_1));
    wait1.wait_for_at_most_seconds(1);

    input_target_listener->focus_changed(mt::fake_shared(surface2));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_2));
    wait2.wait_for_at_most_seconds(1);

    input_target_listener->focus_changed(mt::fake_shared(surface1));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_3));
    wait3.wait_for_at_most_seconds(5);
} 
