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

#include "src/server/input/android/android_input_targeter.h"
#include "src/server/input/android/android_input_registrar.h"
#include "src/server/input/event_filter_chain.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"

#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"
#include "mir_test_framework/fake_event_hub_server_configuration.h"
#include "mir_test_doubles/mock_event_filter.h"
#include "mir_test_doubles/stub_scene_surface.h"
#include "mir_test_doubles/stub_scene.h"
#include "mir_test_doubles/stub_input_enumerator.h"
#include "mir_test/wait_condition.h"
#include "mir_test/event_factory.h"
#include "mir_test/event_matchers.h"

#include "mir/input/event_filter.h"
#include "mir/input/input_channel.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/socket.h>

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
using namespace ::testing;

class AndroidInputManagerAndEventFilterDispatcherSetup : public testing::Test, public mtf::FakeEventHubServerConfiguration
{
public:
    bool is_key_repeat_enabled() const override
    {
        return false;
    }

    std::shared_ptr<mi::CompositeEventFilter> the_composite_event_filter() override
    {
        std::initializer_list<std::shared_ptr<mi::EventFilter>const> const& chain{std::static_pointer_cast<mi::EventFilter>(event_filter)};
        return std::make_shared<mi::EventFilterChain>(chain);
    }
    void SetUp() override
    {
        configuration = the_input_configuration();

        input_manager = configuration->the_input_manager();
        input_manager->start();
        input_dispatcher = the_input_dispatcher();
        input_dispatcher->start();
    }

    void TearDown() override
    {
        input_dispatcher->stop();
        input_manager->stop();
    }

protected:
    std::shared_ptr<mtd::MockEventFilter> event_filter = std::make_shared<mtd::MockEventFilter>();
    std::shared_ptr<mi::InputConfiguration> configuration;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mi::InputDispatcher> input_dispatcher;
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

struct AndroidInputManagerDispatcherInterceptSetup : testing::Test, mtf::FakeEventHubServerConfiguration
{
    bool is_key_repeat_enabled() const override
    {
        return false;
    }

    std::shared_ptr<droidinput::InputEnumerator> the_input_target_enumerator() override
    {
        return std::make_shared<mtd::StubInputEnumerator>();
    }

    std::shared_ptr<droidinput::InputDispatcherPolicyInterface> the_dispatcher_policy() override
    {
        return mt::fake_shared(dispatcher_policy);
    }

    std::shared_ptr<mia::InputRegistrar> the_input_registrar() override
    {
        return mt::fake_shared(input_registrar);
    }

    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        return std::make_shared<mia::InputTargeter>(the_android_input_dispatcher(),
                                                    mt::fake_shared(input_registrar));
    }

    void SetUp() override
    {
        configuration = the_input_configuration();

        input_manager = configuration->the_input_manager();
        input_manager->start();
        input_dispatcher = the_input_dispatcher();
        input_dispatcher->start();
    }

    void TearDown() override
    {
        input_dispatcher->stop();
        input_manager->stop();
    }

    mtd::MockEventFilter event_filter;
    MockDispatcherPolicy dispatcher_policy{mt::fake_shared(event_filter)};
    mtd::StubScene scene;
    mia::InputRegistrar input_registrar{mt::fake_shared(scene)};

    std::shared_ptr<mi::InputConfiguration> configuration;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mi::InputDispatcher> input_dispatcher;

    // TODO: It would be nice if it were possible to mock the interface between
    // droidinput::InputChannel and droidinput::InputDispatcher rather than use
    // valid fds to allow non-throwing construction of a real input channel.
    int test_fd()
    {
        int fds[2];
        // Closed by droidinput InputChannel on shutdown
        socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);
        return fds[0];
    }
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

    mtd::StubSceneSurface surface(test_fd());

    EXPECT_CALL(event_filter, handle(_)).Times(1).WillOnce(Return(false));
    // We return -1 here to skip publishing of the event (to an unconnected test socket!).
    EXPECT_CALL(dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(surface.fd), _, _))
        .Times(1).WillOnce(DoAll(mt::WakeUp(&wait_condition), Return(-1)));

    input_registrar.add_window_handle_for_surface(&surface);
    the_input_targeter()->focus_changed(surface.input_channel());

    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_ENTER));

    wait_condition.wait_for_at_most_seconds(1);
}

TEST_F(AndroidInputManagerDispatcherInterceptSetup, changing_focus_changes_event_recipient)
{
    using namespace ::testing;

    mt::WaitCondition wait1, wait2, wait3;

    mtd::StubSceneSurface surface1(test_fd());
    mtd::StubSceneSurface surface2(test_fd());

    input_registrar.add_window_handle_for_surface(&surface1);
    input_registrar.add_window_handle_for_surface(&surface2);

    EXPECT_CALL(event_filter, handle(_)).Times(3).WillRepeatedly(Return(false));

    {
        InSequence seq;

        EXPECT_CALL(dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(surface1.fd), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait1), Return(-1)));
        EXPECT_CALL(dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(surface2.fd), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait2), Return(-1)));
        EXPECT_CALL(dispatcher_policy, interceptKeyBeforeDispatching(WindowHandleWithInputFd(surface1.fd), _, _))
            .Times(1).WillOnce(DoAll(mt::WakeUp(&wait3), Return(-1)));
    }

    the_input_targeter()->focus_changed(surface1.input_channel());
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_1));
    wait1.wait_for_at_most_seconds(1);

    the_input_targeter()->focus_changed(surface2.input_channel());
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_2));
    wait2.wait_for_at_most_seconds(1);

    the_input_targeter()->focus_changed(surface1.input_channel());
    fake_event_hub->synthesize_event(mis::a_key_down_event()
                                .of_scancode(KEY_3));
    wait3.wait_for_at_most_seconds(5);
}
