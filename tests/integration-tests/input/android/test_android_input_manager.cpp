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
#include "mir/input/input_targets.h"
#include "mir/input/input_region.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/input/null_input_report.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_targeter.h"
#include "src/server/input/android/android_input_registrar.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/mock_input_surface.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"
#include "mir_test/event_matchers.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputEnumerator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/socket.h>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using mtd::MockEventFilter;

namespace
{
using namespace ::testing;


static const std::shared_ptr<mi::CursorListener> null_cursor_listener{};

struct StubInputTargets : public mi::InputTargets
{
    void for_each(std::function<void(std::shared_ptr<mi::InputChannel> const&)> const&)
    {
    }
};

struct StubInputRegion : public mi::InputRegion
{
    geom::Rectangle bounding_rectangle()
    {
        return {geom::Point{}, geom::Size{1600, 1400}};
    }

    void confine(geom::Point&)
    {
    }
};

class AndroidInputManagerAndEventFilterDispatcherSetup : public testing::Test
{
public:
    AndroidInputManagerAndEventFilterDispatcherSetup()
    {
        event_filter = std::make_shared<MockEventFilter>();
        configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(
                event_filter,
                mt::fake_shared(input_region),
                null_cursor_listener,
                std::make_shared<mi::NullInputReport>());

        fake_event_hub = configuration->the_fake_event_hub();
        
        input_manager = configuration->the_input_manager();

        stub_targets = std::make_shared<StubInputTargets>();
        configuration->set_input_targets(stub_targets);

        input_manager->start();
    }

    void TearDown()
    {
        input_manager->stop();
    }

  protected:
    std::shared_ptr<mtd::FakeEventHubInputConfiguration> configuration;
    mia::FakeEventHub* fake_event_hub;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<MockEventFilter> event_filter;
    StubInputRegion input_region;
    std::shared_ptr<StubInputTargets> stub_targets;
};

}

TEST_F(AndroidInputManagerAndEventFilterDispatcherSetup, manager_dispatches_key_events_to_filter)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    EXPECT_CALL(
        *event_filter,
        handle(mt::KeyDownEvent()))
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
        handle(mt::ButtonDownEvent()))
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
          handle(mt::ButtonDownEvent()))
              .Times(1)
              .WillOnce(Return(false));
      EXPECT_CALL(
          *event_filter,
          handle(mt::ButtonUpEvent()))
              .Times(1)
              .WillOnce(Return(false));
      EXPECT_CALL(
          *event_filter,
          handle(mt::MotionEvent(0,0)))
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
                    handle(mt::MotionEvent(100, 100)))
            .WillOnce(Return(false));
        EXPECT_CALL(*event_filter,
                    handle(mt::MotionEvent(200, 100)))
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
                              std::shared_ptr<mi::InputRegion> const& input_region,
                              std::shared_ptr<mi::CursorListener> const& cursor_listener,
                              std::shared_ptr<mi::InputReport> const& input_report)
        : FakeEventHubInputConfiguration({}, input_region, cursor_listener, input_report),
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
            mt::fake_shared(input_region), null_cursor_listener, std::make_shared<mi::NullInputReport>());
        fake_event_hub = configuration->the_fake_event_hub();

        input_manager = configuration->the_input_manager();
        
        input_registrar = configuration->the_input_registrar();
        input_targeter = configuration->the_input_targeter();

        dispatcher_policy = configuration->the_mock_dispatcher_policy();

        stub_targets = std::make_shared<StubInputTargets>();
        configuration->set_input_targets(stub_targets);
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
    StubInputRegion input_region;
    std::shared_ptr<TestingInputConfiguration> configuration;
    mia::FakeEventHub* fake_event_hub;
    droidinput::sp<MockDispatcherPolicy> dispatcher_policy;

    std::shared_ptr<StubInputTargets> stub_targets;

    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<ms::InputRegistrar> input_registrar;
    std::shared_ptr<msh::InputTargeter> input_targeter;
};

MATCHER_P(WindowHandleWithInputFd, input_fd, "")
{
    if (arg->getInputChannel()->getFd() == input_fd)
        return true;
    return false;
}

}

TEST_F(AndroidInputManagerDispatcherInterceptSetup, server_input_fd_of_focused_channel_is_sent_unfiltered_key_events)
{
    using namespace ::testing;

    mt::WaitCondition wait_condition;

    auto input_fd = test_fd();
    mtd::StubInputChannel channel(input_fd);
    mtd::StubInputSurface surface;

    EXPECT_CALL(*event_filter, handle(_)).Times(1).WillOnce(Return(false));
    // We return -1 here to skip publishing of the event (to an unconnected test socket!).
    EXPECT_CALL(*dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(input_fd), _, _))
        .Times(1).WillOnce(DoAll(mt::WakeUp(&wait_condition), Return(-1)));

    input_registrar->input_channel_opened(mt::fake_shared(channel), mt::fake_shared(surface), mi::InputReceptionMode::normal);
    input_targeter->focus_changed(mt::fake_shared(channel));

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

    mtd::StubInputSurface surface;
    auto input_fd_1 = test_fd();
    mtd::StubInputChannel channel1(input_fd_1);
    auto input_fd_2 = test_fd();
    mtd::StubInputChannel channel2(input_fd_2);

    input_registrar->input_channel_opened(mt::fake_shared(channel1), mt::fake_shared(surface), mi::InputReceptionMode::normal);
    input_registrar->input_channel_opened(mt::fake_shared(channel2), mt::fake_shared(surface), mi::InputReceptionMode::normal);

    EXPECT_CALL(*event_filter, handle(_)).Times(3).WillRepeatedly(Return(false));

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

    input_targeter->focus_changed(mt::fake_shared(channel1));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_1));
    wait1.wait_for_at_most_seconds(1);

    input_targeter->focus_changed(mt::fake_shared(channel2));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_2));
    wait2.wait_for_at_most_seconds(1);

    input_targeter->focus_changed(mt::fake_shared(channel1));
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_3));
    wait3.wait_for_at_most_seconds(5);
} 
