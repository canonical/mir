/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/server/input/default_event_builder.h"
#include "src/platforms/mesa/server/x11/input/input_platform.h"
#include "src/platforms/mesa/server/x11/input/input_device.h"

#include "mir/events/event_private.h"
#include "mir/dispatch/dispatchable.h"
#include "mir_toolkit/event.h"
#include "mir_toolkit/events/input/input_event.h"
#include "mir/test/doubles/mock_input_sink.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_input_device_registry.h"
#include "mir/test/doubles/mock_x11.h"
#include "mir/test/fake_shared.h"
#include "mir/cookie/authority.h"
#include "mir/test/event_matchers.h"

namespace md = mir::dispatch;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{

struct X11PlatformTest : ::testing::Test
{
    NiceMock<mtd::MockInputSink> mock_pointer_sink;
    NiceMock<mtd::MockInputSink> mock_keyboard_sink;
    NiceMock<mtd::MockInputSeat> mock_seat;
    NiceMock<mtd::MockX11> mock_x11;
    NiceMock<mtd::MockInputDeviceRegistry> mock_registry;
    mir::input::DefaultEventBuilder builder{0, mir::cookie::Authority::create(), mt::fake_shared(mock_seat)};

    mir::input::X::XInputPlatform x11_platform{
        mt::fake_shared(mock_registry),
        std::shared_ptr<::Display>(
            XOpenDisplay(nullptr),
            [](::Display* display) { XCloseDisplay(display); })};

    std::vector<std::shared_ptr<mi::InputDevice>> devices;
    enum class GrabState { FOCUSIN, FOCUSOUT, ENTERNOTIFY, LEAVENOTIFY };
    GrabState grab_state{GrabState::FOCUSIN};

    void capture_devices()
    {
        ON_CALL(mock_registry, add_device(_))
            .WillByDefault(Invoke([this](auto const& dev)
                                  {
                                      devices.push_back(dev);
                                      auto const info = dev->get_device_info();
                                      if (contains(info.capabilities, mi::DeviceCapability::pointer))
                                          dev->start(&mock_pointer_sink, &builder);
                                      else if (contains(info.capabilities, mi::DeviceCapability::keyboard))
                                          dev->start(&mock_keyboard_sink, &builder);
                                  })
                          );
    }

    void prepare_event_processing()
    {
        capture_devices();
        x11_platform.start();
    }


    void process_input_event()
    {
        x11_platform.dispatchable()->dispatch(md::FdEvent::readable);
    }
};

}

MATCHER(ButtonUpEventWithNoButtonsPressed, "")
{
    auto pev = mt::maybe_pointer_event(mt::to_address(arg));
    return mt::button_event_matches(pev, 0, 0, mir_pointer_action_button_up, 0, true, true, false);
}

TEST_F(X11PlatformTest, registers_two_devices)
{
    EXPECT_CALL(mock_registry, add_device(_)).Times(2);

    x11_platform.start();
}

TEST_F(X11PlatformTest, unregisters_devices_on_stop)
{
    EXPECT_CALL(mock_registry, add_device(_)).Times(2);
    EXPECT_CALL(mock_registry, remove_device(_)).Times(2);

    x11_platform.start();
    x11_platform.stop();
}

TEST_F(X11PlatformTest, registered_devices_mimic_mouse_and_keyboard)
{
    capture_devices();

    x11_platform.start();

    ASSERT_THAT(devices.size(), Eq(2u));

    auto const& first_info = devices[0]->get_device_info();
    auto const& second_info = devices[1]->get_device_info();

    EXPECT_FALSE(first_info.capabilities == second_info.capabilities);
    EXPECT_TRUE(contains(first_info.capabilities | second_info.capabilities,
                         mi::DeviceCapability::pointer));
    EXPECT_TRUE(contains(first_info.capabilities | second_info.capabilities,
                         mi::DeviceCapability::keyboard));

}

TEST_F(X11PlatformTest, dispatches_input_events_to_sink)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.keypress_event_return),
                       Return(1)));

    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(1));

    process_input_event();
}

TEST_F(X11PlatformTest, button_release_has_no_buttons_pressed)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.button_release_event_return),
                       Return(1)));

    EXPECT_CALL(mock_pointer_sink, handle_input(ButtonUpEventWithNoButtonsPressed()))
        .Times(Exactly(1));

    process_input_event();
}

TEST_F(X11PlatformTest, button4_button5_converted_to_vscroll)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.vscroll_event_return),
                       Return(1)));

    EXPECT_CALL(mock_pointer_sink, handle_input(mt::PointerMovementEvent()))
        .Times(Exactly(1));

    process_input_event();
}

TEST_F(X11PlatformTest, motion_event_trigger_pointer_movement)
{
    mock_x11.fake_x11.pending_events = 2;

    prepare_event_processing();

    InSequence seq;
    mock_x11.fake_x11.motion_event_return.xmotion.x = 20;
    mock_x11.fake_x11.motion_event_return.xmotion.y = 20;
    EXPECT_CALL(mock_x11, XNextEvent(_,_))
        .WillOnce(DoAll(SetArgPointee<1>(mock_x11.fake_x11.motion_event_return),
                       Return(1)));
    EXPECT_CALL(mock_pointer_sink, handle_input(mt::PointerEventWithDiff(20, 20)));
    mock_x11.fake_x11.motion_event_return.xmotion.x = 40;
    mock_x11.fake_x11.motion_event_return.xmotion.y = 25;
    EXPECT_CALL(mock_x11, XNextEvent(_,_))
        .WillOnce(DoAll(SetArgPointee<1>(mock_x11.fake_x11.motion_event_return),
                       Return(1)));
    EXPECT_CALL(mock_pointer_sink, handle_input(mt::PointerEventWithDiff(20, 5)));

    process_input_event();
    process_input_event();
}

TEST_F(X11PlatformTest, grabs_keyboard)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.focus_in_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XGrabKeyboard(_,_,_,_,_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
}

TEST_F(X11PlatformTest, ungrabs_keyboard)
{
    mock_x11.fake_x11.pending_events = 2;

    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(Invoke([this](Display*, XEvent* xev)
                              {
                                  *xev = (grab_state == GrabState::FOCUSIN)
                                             ? mock_x11.fake_x11.focus_in_event_return
                                             : mock_x11.fake_x11.focus_out_event_return;
                                  grab_state = GrabState::FOCUSOUT;
                                  return 1;
                              }));

    EXPECT_CALL(mock_x11, XUngrabKeyboard(_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
    process_input_event();
}

TEST_F(X11PlatformTest, does_not_ungrab_keyboard_without_grabbing_first)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.focus_out_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XUngrabKeyboard(_,_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
}

TEST_F(X11PlatformTest, does_not_grab_pointer_unfocussed)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.enter_notify_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XGrabPointer(_,_,_,_,_,_,_,_,_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
}

TEST_F(X11PlatformTest, grabs_pointer_when_focussed)
{
    mock_x11.fake_x11.pending_events = 2;

    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(Invoke([this](Display*, XEvent* xev)
                              {
                                  *xev = (grab_state == GrabState::FOCUSIN)
                                         ? mock_x11.fake_x11.focus_in_event_return
                                         : mock_x11.fake_x11.enter_notify_event_return;
                                  grab_state = GrabState::ENTERNOTIFY;
                                  return 1;
                              }));

    EXPECT_CALL(mock_x11, XGrabPointer(_,_,_,_,_,_,_,_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
    process_input_event();
}

TEST_F(X11PlatformTest, does_not_ungrab_pointer_without_grabbing_first)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.leave_notify_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XUngrabPointer(_,_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
}

TEST_F(X11PlatformTest, ungrabs_pointer)
{
    mock_x11.fake_x11.pending_events = 3;

    prepare_event_processing();

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(Invoke([this](Display*, XEvent* xev)
                              {
                                  switch(grab_state)
                                  {
                                  case GrabState::FOCUSIN:
                                      *xev = mock_x11.fake_x11.focus_in_event_return;
                                      grab_state = GrabState::ENTERNOTIFY;
                                      break;
                                  case GrabState::ENTERNOTIFY:
                                      *xev = mock_x11.fake_x11.enter_notify_event_return;
                                      grab_state = GrabState::LEAVENOTIFY;
                                      break;
                                  case GrabState::LEAVENOTIFY:
                                      *xev = mock_x11.fake_x11.leave_notify_event_return;
                                      break;
                                  case GrabState::FOCUSOUT: //only needed to appease the compiler.
                                      break;
                                  }

                                  return 1;
                              }));

    EXPECT_CALL(mock_x11, XUngrabKeyboard(_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_pointer_sink, handle_input(_))
        .Times(Exactly(0));
    EXPECT_CALL(mock_keyboard_sink, handle_input(_))
        .Times(Exactly(0));

    process_input_event();
    process_input_event();
    process_input_event();
}

TEST_F(X11PlatformTest, does_not_block_on_events)
{
    prepare_event_processing();

    ON_CALL(mock_x11, XPending(_))
    .WillByDefault(Return(0));

    EXPECT_CALL(mock_x11, XNextEvent(_,_))
    .Times(Exactly(0));

    process_input_event();
}

TEST_F(X11PlatformTest, enter_hides_X_cursor)
{
    XEvent events[] =
        {
            mock_x11.fake_x11.focus_in_event_return,
            mock_x11.fake_x11.enter_notify_event_return,
        };

    mock_x11.fake_x11.pending_events = std::distance(std::begin(events), std::end(events));
    prepare_event_processing();

    auto const* next_event = std::begin(events);

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(Invoke([this, &next_event](Display*, XEvent* xev)
        {
            *xev = *next_event++;

            return 1;
        }));

    EXPECT_CALL(mock_x11, XFixesHideCursor(_,_))
        .Times(Exactly(1));

    for (auto unused: events)
    {
        (void)unused;
        process_input_event();
    }
}

TEST_F(X11PlatformTest, leave_shows_X_cursor)
{
    XEvent events[] =
        {
            mock_x11.fake_x11.focus_in_event_return,
            mock_x11.fake_x11.enter_notify_event_return,
            mock_x11.fake_x11.leave_notify_event_return,
        };

    mock_x11.fake_x11.pending_events = std::distance(std::begin(events), std::end(events));
    prepare_event_processing();

    auto const* next_event = std::begin(events);

    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(Invoke([&next_event](Display*, XEvent* xev)
        {
            *xev = *next_event++;

            return 1;
        }));

    EXPECT_CALL(mock_x11, XFixesShowCursor(_,_))
        .Times(Exactly(1));

    for (auto unused: events)
    {
        (void)unused;
        process_input_event();
    }
}
