/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/platforms/evdev/libinput_device.h"
#include "src/platforms/evdev/button_utils.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/input/default_event_builder.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/flags.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_input_sink.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/fake_shared.h"
#include "mir/udev/wrapper.h"
#include "mir_test_framework/libinput_environment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>
#include <libinput.h>

#include <chrono>
#include <gtest/internal/gtest-internal.h>

namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mev = mir::events;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace mir
{
namespace events
{
template<typename Tag>
std::ostream& operator<<(std::ostream& out, mev::ScrollAxisV1<Tag> const& axis)
{
    return out
        << "(precise: " << axis.precise
        << ", discrete: " << axis.discrete
        << ", value120: " << axis.value120
        << ", stop: " << axis.stop << ")";
}
}
}

namespace
{
using namespace ::testing;
using Matrix = mi::OutputInfo::Matrix;

struct MockEventBuilder : mi::EventBuilder
{
    mtd::MockInputSeat seat;
    mtd::AdvanceableClock clock;
    mi::DefaultEventBuilder builder{
        MirInputDeviceId{3},
        mt::fake_shared(clock)};
    MockEventBuilder()
    {
        ON_CALL(*this, key_event(_,_,_,_)).WillByDefault(
            [this](std::optional<Timestamp> time, MirKeyboardAction action, xkb_keysym_t key, int scan_code)
            {
                return builder.key_event(time, action, key, scan_code);
            });

        ON_CALL(*this, touch_event(_, _)).WillByDefault(
            [this](std::optional<Timestamp> time, std::vector<mir::events::TouchContact> const& contacts)
            {
                return builder.touch_event(time, contacts);
            });

        ON_CALL(*this, pointer_event(_, _, _, _, _, _, _, _)).WillByDefault(
            [this](
                std::optional<Timestamp> timestamp,
                MirPointerAction action,
                MirPointerButtons buttons,
                std::optional<mir::geometry::PointF> position,
                mir::geometry::DisplacementF motion,
                MirPointerAxisSource axis_source,
                mir::events::ScrollAxisH h_scroll,
                mir::events::ScrollAxisV v_scroll)
            {
                return builder.pointer_event(
                    timestamp, action, buttons,
                    position, motion,
                    axis_source, h_scroll, v_scroll);
            });
    }
    using EventBuilder::Timestamp;

    MOCK_METHOD(mir::EventUPtr, key_event, (std::optional<Timestamp>, MirKeyboardAction, xkb_keysym_t, int));

    mir::EventUPtr touch_event(std::optional<Timestamp>, std::vector<mir::events::TouchContactV1> const&)
    {
        BOOST_THROW_EXCEPTION(std::logic_error{"Deprecated touch_event() called"});
    }

    MOCK_METHOD(mir::EventUPtr, touch_event, (std::optional<Timestamp>, std::vector<mir::events::TouchContactV2> const&));
    MOCK_METHOD(mir::EventUPtr, pointer_event,
                (std::optional<Timestamp>, MirPointerAction, MirPointerButtons, std::optional<mir::geometry::PointF>,
                 mir::geometry::DisplacementF, MirPointerAxisSource, mir::events::ScrollAxisH,
                 mir::events::ScrollAxisV), (override));

    mir::EventUPtr pointer_event(
        std::optional<Timestamp>,
        MirPointerAction,
        MirPointerButtons,
        float, float,
        float, float) override
    {
        BOOST_THROW_EXCEPTION(std::logic_error("deprecated event builder method called"));
    }

    mir::EventUPtr pointer_event(
        std::optional<Timestamp>,
        MirPointerAction,
        MirPointerButtons,
        float, float,
        float, float,
        float, float) override
    {
        BOOST_THROW_EXCEPTION(std::logic_error("deprecated event builder method called"));
    }

    mir::EventUPtr pointer_axis_event(
        MirPointerAxisSource,
        std::optional<Timestamp>,
        MirPointerAction,
        MirPointerButtons,
        float, float,
        float, float,
        float, float) override
    {
        BOOST_THROW_EXCEPTION(std::logic_error("deprecated event builder method called"));
    }

    mir::EventUPtr pointer_axis_with_stop_event(
        MirPointerAxisSource,
        std::optional<Timestamp>,
        MirPointerAction,
        MirPointerButtons,
        float, float,
        float, float,
        bool, bool,
        float, float) override
    {
        BOOST_THROW_EXCEPTION(std::logic_error("deprecated event builder method called"));
    }

    mir::EventUPtr pointer_axis_discrete_scroll_event(
        MirPointerAxisSource,
        std::optional<Timestamp>,
        MirPointerAction,
        MirPointerButtons,
        float, float,
        float, float) override
    {
        BOOST_THROW_EXCEPTION(std::logic_error("deprecated event builder method called"));
    }
};

struct LibInputDevice : public ::testing::Test
{
    mtf::LibInputEnvironment env;
    ::testing::NiceMock<mtd::MockInputSink> mock_sink;
    ::testing::NiceMock<MockEventBuilder> mock_builder;
    std::shared_ptr<libinput> lib;

    const uint64_t event_time_1 = 1000;
    const mi::EventBuilder::Timestamp time_stamp_1{std::chrono::microseconds{event_time_1}};
    const uint64_t event_time_2 = 2000;
    const mi::EventBuilder::Timestamp time_stamp_2{std::chrono::microseconds{event_time_2}};
    const uint64_t event_time_3 = 3000;
    const mi::EventBuilder::Timestamp time_stamp_3{std::chrono::microseconds{event_time_3}};
    const uint64_t event_time_4 = 4000;
    const mi::EventBuilder::Timestamp time_stamp_4{std::chrono::microseconds{event_time_4}};

    udev *const fake_udev = reinterpret_cast<udev*>(0xFACE01);

    LibInputDevice()
    {
        ON_CALL(mock_sink, bounding_rectangle())
            .WillByDefault(Return(geom::Rectangle({0,0}, {100,100})));
        lib = mie::make_libinput(nullptr);
    }

    libinput_device* setup_laptop_keyboard()
    {
        return env.setup_device(mtf::LibInputEnvironment::laptop_keyboard);
    }

    libinput_device* setup_trackpad()
    {
        return env.setup_device(mtf::LibInputEnvironment::bluetooth_magic_trackpad);
    }

    libinput_device* setup_touchscreen()
    {
        return env.setup_device(mtf::LibInputEnvironment::mtk_tpd);
    }

    libinput_device* setup_touchpad()
    {
        return env.setup_device(mtf::LibInputEnvironment::synaptics_touchpad);
    }

    libinput_device* setup_mouse()
    {
        return env.setup_device(mtf::LibInputEnvironment::usb_mouse);
    }

    void setup_pointer_configuration(libinput_device* dev, double accel_speed, MirPointerHandedness handedness, MirPointerAcceleration profile)
    {
        ON_CALL(env.mock_libinput, libinput_device_config_accel_get_speed(dev))
            .WillByDefault(Return(accel_speed));
        ON_CALL(env.mock_libinput, libinput_device_config_left_handed_get(dev))
            .WillByDefault(Return(handedness == mir_pointer_handedness_left));
        ON_CALL(env.mock_libinput, libinput_device_config_accel_get_profile(dev))
            .WillByDefault(Return((profile == mir_pointer_acceleration_none) ?
                                      LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT :
                                      LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE));
    }

    void setup_touchpad_configuration(libinput_device* dev,
                                       MirTouchpadClickMode click_mode,
                                       MirTouchpadScrollMode scroll_mode,
                                       int scroll_button,
                                       bool tap_to_click,
                                       bool disable_while_typing,
                                       bool disable_with_mouse,
                                       bool middle_button_emulation)
    {
        mir::Flags<libinput_config_click_method> click_method = LIBINPUT_CONFIG_CLICK_METHOD_NONE;
        if (click_mode & mir_touchpad_click_mode_finger_count)
            click_method |= LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER;
        if (click_mode & mir_touchpad_click_mode_area_to_click)
            click_method |= LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

        mir::Flags<libinput_config_scroll_method> scroll_method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
        if (scroll_mode & mir_touchpad_scroll_mode_two_finger_scroll)
            scroll_method |= LIBINPUT_CONFIG_SCROLL_2FG;
        if (scroll_mode & mir_touchpad_scroll_mode_edge_scroll)
            scroll_method |= LIBINPUT_CONFIG_SCROLL_EDGE;
        if (scroll_mode & mir_touchpad_scroll_mode_button_down_scroll)
            scroll_method |= LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;

        ON_CALL(env.mock_libinput, libinput_device_config_click_get_method(dev))
            .WillByDefault(Return(static_cast<libinput_config_click_method>(click_method.value())));
        ON_CALL(env.mock_libinput, libinput_device_config_scroll_get_method(dev))
            .WillByDefault(Return(static_cast<libinput_config_scroll_method>(scroll_method.value())));
        ON_CALL(env.mock_libinput, libinput_device_config_scroll_get_button(dev))
            .WillByDefault(Return(scroll_button));
        ON_CALL(env.mock_libinput, libinput_device_config_tap_get_enabled(dev))
            .WillByDefault(Return(tap_to_click?
                                   LIBINPUT_CONFIG_TAP_ENABLED:
                                   LIBINPUT_CONFIG_TAP_DISABLED));
        ON_CALL(env.mock_libinput, libinput_device_config_dwt_get_enabled(dev))
            .WillByDefault(Return(disable_while_typing?
                                   LIBINPUT_CONFIG_DWT_ENABLED:
                                   LIBINPUT_CONFIG_DWT_DISABLED));
        ON_CALL(env.mock_libinput, libinput_device_config_send_events_get_mode(dev))
            .WillByDefault(Return(disable_with_mouse?
                                   LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE:
                                   LIBINPUT_CONFIG_SEND_EVENTS_ENABLED));
        ON_CALL(env.mock_libinput, libinput_device_config_middle_emulation_get_enabled(dev))
            .WillByDefault(Return(middle_button_emulation?
                                   LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED:
                                   LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED));

    }

    void process_events(mie::LibInputDevice& device)
    {
        for (auto event : env.mock_libinput.events)
        {
            device.process_event(event);
        }
        env.mock_libinput.events.clear();
    }
};

struct LibInputDeviceOnLaptopKeyboard : public LibInputDevice
{
    libinput_device*const fake_device = setup_laptop_keyboard();
    mie::LibInputDevice keyboard{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device)};
};

struct LibInputDeviceOnMouse : public LibInputDevice
{
    libinput_device*const fake_device = setup_mouse();
    mie::LibInputDevice mouse{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device)};
};

struct LibInputDeviceOnLaptopKeyboardAndMouse : public LibInputDevice
{
    libinput_device*const fake_device = setup_mouse();
    libinput_device*const fake_device_2 = setup_laptop_keyboard();
    mie::LibInputDevice keyboard{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device_2)};
    mie::LibInputDevice mouse{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device)};
};

struct LibInputDeviceOnTouchScreen : public LibInputDevice
{
    libinput_device*const fake_device = setup_touchscreen();
    mie::LibInputDevice touch_screen{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device)};

    const float x = 10;
    const float y = 5;
    const uint32_t output_id = 2;
    const float output_x_pos = 20;
    const float output_y_pos = 50;
    const float screen_x_pos = 70;
    const float screen_y_pos = 30;
    const int width = 100;
    const int height = 200;

    void SetUp() override
    {
        LibInputDevice::SetUp();
        ON_CALL(mock_sink, output_info(0))
            .WillByDefault(Return(
                mi::OutputInfo{
                    true,
                    geom::Size{width, height},
                    Matrix{{1.0f, 0.0f, 0, 0.0f, 1.0f, 0}}}));
    }
};

struct LibInputDeviceOnTouchpad : public LibInputDevice
{
    libinput_device*const fake_device = setup_touchpad();
    mie::LibInputDevice touchpad{mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device)};
};
}

TEST_F(LibInputDevice, start_creates_and_refs_libinput_device)
{
    auto * const fake_device = setup_laptop_keyboard();

    // according to manual when a new device is detected by udev libinput creates a temporary
    // device with a ref count 0, which gets distributed via its event loop, and would be removed
    // after event dispatch. So it needs a manual ref call
    EXPECT_CALL(env.mock_libinput, libinput_device_ref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            mie::make_libinput_device(lib, fake_device));
    dev.start(&mock_sink, &mock_builder);
}

TEST_F(LibInputDevice, unique_id_contains_device_name)
{
    auto fake_device = env.mock_libinput.get_next_fake_ptr<libinput_device*>();
    auto fake_dev_group = env.mock_libinput.get_next_fake_ptr<libinput_device_group*>();
    auto udev_dev = std::shared_ptr<udev_device>{
        env.mock_libinput.get_next_fake_ptr<udev_device*>(),
        [](auto){}};
    env.mock_libinput.setup_device(fake_device, fake_dev_group, udev_dev, "Keyboard", "event4", 1, 2);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            mie::make_libinput_device(lib, fake_device));
    auto info = dev.get_device_info();

    EXPECT_THAT(info.unique_id, Eq("Keyboard-event4-1-2"));
}

TEST_F(LibInputDevice, removal_unrefs_libinput_device)
{
    auto fake_device = setup_laptop_keyboard();

    EXPECT_CALL(env.mock_libinput, libinput_device_unref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(), mie::make_libinput_device(lib, fake_device));
}

TEST_F(LibInputDeviceOnLaptopKeyboard, process_event_converts_key_event)
{
    EXPECT_CALL(mock_builder, key_event({time_stamp_1}, mir_keyboard_action_down, _, KEY_A));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event({time_stamp_2}, mir_keyboard_action_up, _, KEY_A));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyUpEvent())));

    keyboard.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_key_event(fake_device, event_time_1, KEY_A, LIBINPUT_KEY_STATE_PRESSED);
    env.mock_libinput.setup_key_event(fake_device, event_time_2, KEY_A, LIBINPUT_KEY_STATE_RELEASED);
    process_events(keyboard);
}

TEST_F(LibInputDeviceOnLaptopKeyboard, process_event_accumulates_key_state)
{
    InSequence seq;
    EXPECT_CALL(mock_builder, key_event({time_stamp_1}, mir_keyboard_action_down, _, KEY_C));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event({time_stamp_2}, mir_keyboard_action_down, _, KEY_LEFTALT));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_LEFTALT),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event({time_stamp_3}, mir_keyboard_action_up, _, KEY_C));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),
                                              mt::KeyUpEvent())));

    keyboard.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_key_event(fake_device, event_time_1, KEY_C, LIBINPUT_KEY_STATE_PRESSED);
    env.mock_libinput.setup_key_event(fake_device, event_time_2, KEY_LEFTALT, LIBINPUT_KEY_STATE_PRESSED);
    env.mock_libinput.setup_key_event(fake_device, event_time_3, KEY_C, LIBINPUT_KEY_STATE_RELEASED);
    process_events(keyboard);
}

TEST_F(LibInputDeviceOnMouse, process_event_converts_pointer_event)
{
    float x_movement_1 = 15;
    float y_movement_1 = 17;
    float x_movement_2 = 20;
    float y_movement_2 = 40;

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x_movement_1,y_movement_1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x_movement_2,y_movement_2)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_1, x_movement_1, y_movement_1);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_2, x_movement_2, y_movement_2);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_absolute_pointer_events)
{
    float x1 = 15;
    float y1 = 17;
    float x2 = 40;
    float y2 = 10;

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x1, y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x2 - x1, y2 - y1)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_absolute_pointer_event(fake_device, event_time_1, x1, y1);
    env.mock_libinput.setup_absolute_pointer_event(fake_device, event_time_2, x2, y2);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_event_motion_events_with_relative_changes)
{
    float x1 = 15, x2 = 23;
    float y1 = 17, y2 = 21;

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x1,y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x2,y2)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_1, x1, y1);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_2, x2, y2);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, notices_slow_relative_movements)
{   // Regression test for LP: #1528109
    float x1 = 0.5f, x2 = 0.6f;
    float y1 = 1.7f, y2 = 1.8f;

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x1,y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x2,y2)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_1, x1, y1);
    env.mock_libinput.setup_pointer_event(fake_device, event_time_2, x2, y2);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_press_and_release)
{
    float const x = 0;
    float const y = 0;
    geom::Point const pos{x, y};

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_primary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_primary)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_button_event(fake_device, event_time_1, BTN_LEFT, LIBINPUT_BUTTON_STATE_PRESSED);
    env.mock_libinput.setup_button_event(fake_device, event_time_2, BTN_RIGHT, LIBINPUT_BUTTON_STATE_PRESSED);
    env.mock_libinput.setup_button_event(fake_device, event_time_3, BTN_RIGHT, LIBINPUT_BUTTON_STATE_RELEASED);
    env.mock_libinput.setup_button_event(fake_device, event_time_4, BTN_LEFT, LIBINPUT_BUTTON_STATE_RELEASED);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_exotic_mouse_buttons)
{
    float const x = 0;
    float const y = 0;
    geom::Point const pos{x, y};

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_side)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_extra)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_task)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_button_event(fake_device, event_time_1, BTN_SIDE, LIBINPUT_BUTTON_STATE_PRESSED);
    env.mock_libinput.setup_button_event(fake_device, event_time_2, BTN_EXTRA, LIBINPUT_BUTTON_STATE_PRESSED);
    env.mock_libinput.setup_button_event(fake_device, event_time_3, BTN_TASK, LIBINPUT_BUTTON_STATE_PRESSED);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_ignores_events_when_button_conversion_fails)
{
    EXPECT_THROW({mir::input::evdev::to_pointer_button(BTN_JOYSTICK, mir_pointer_handedness_right);},
                 std::runtime_error);

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(_)).Times(0);
    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_button_event(fake_device, event_time_1, BTN_JOYSTICK, LIBINPUT_BUTTON_STATE_PRESSED);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_scroll)
{
    InSequence seq;
    // expect two scroll events..

    mev::ScrollAxisV const scroll_axis_v_1{geom::DeltaYF{-20}, geom::DeltaY{2}, geom::DeltaY{240}, false};
    mev::ScrollAxisH const scroll_axis_h_2{geom::DeltaXF{5}, geom::DeltaX{1}, geom::DeltaX{120}, false};

    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_1}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_wheel,
        mev::ScrollAxisH{},
        scroll_axis_v_1));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, -20.0f)));
    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_2}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_wheel,
        scroll_axis_h_2,
        mev::ScrollAxisV{{}, {}, false}));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 5.0f)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, -20.0, 0, 240);
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_2, 5.0, {}, 120, 0);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_is_picked_up)
{
    auto const expected_v120 = 165;
    auto const expected_discrete = 1;

    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_1}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_wheel,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{geom::DeltaYF{1}, geom::DeltaY{expected_discrete}, geom::DeltaY{expected_v120}, false}));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, 1.0f, 0.0f, 165);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_does_not_combine_with_precise)
{
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 1.0f)));

    mouse.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, 1.0f, 0.0f, 250.0f);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_scroll_can_be_multiple_discrete_steps)
{
    mouse.start(&mock_sink, &mock_builder);

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{1}, geom::DeltaY{2}, geom::DeltaY{240}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, 1.0f, 0.0f, 240.0f);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_scroll_can_be_negative)
{
    mouse.start(&mock_sink, &mock_builder);

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{-1}, geom::DeltaY{-2}, geom::DeltaY{-240}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, -1.0f, 0.0f, -240.0f);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_scroll_is_accumulated)
{
    mouse.start(&mock_sink, &mock_builder);

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{1}, geom::DeltaY{0}, geom::DeltaY{50}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, 1.0f, 0.0f, 50.0f);
    process_events(mouse);

    Mock::VerifyAndClearExpectations(&mock_builder);
    ASSERT_THAT(env.mock_libinput.events.size(), Eq(0));

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{1}, geom::DeltaY{0}, geom::DeltaY{50}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_2, {}, 1.0f, 0.0f, 50.0f);
    process_events(mouse);

    Mock::VerifyAndClearExpectations(&mock_builder);
    ASSERT_THAT(env.mock_libinput.events.size(), Eq(0));

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{1}, geom::DeltaY{1}, geom::DeltaY{50}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_3, {}, 1.0f, 0.0f, 50.0f);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnMouse, hi_res_scroll_scroll_is_accumulated_negative)
{
    mouse.start(&mock_sink, &mock_builder);

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{-1}, geom::DeltaY{-2}, geom::DeltaY{-350}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_1, {}, -1.0f, 0.0f, -350.0f);
    process_events(mouse);

    Mock::VerifyAndClearExpectations(&mock_builder);
    ASSERT_THAT(env.mock_libinput.events.size(), Eq(0));

    EXPECT_CALL(mock_builder, pointer_event(
        _, _, _, _, _, _, _,
        mev::ScrollAxisV{geom::DeltaYF{-1}, geom::DeltaY{-1}, geom::DeltaY{-10}, false}));
    env.mock_libinput.setup_pointer_scroll_wheel_event(fake_device, event_time_2, {}, -1.0f, 0.0f, -10.0f);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_ignores_uncorrelated_touch_up_events)
{
    MirTouchId slot = 3;

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(0);
    EXPECT_CALL(mock_sink, handle_input(_)).Times(0);

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_ignores_excessive_touch_events_on_coordinate_0_0)
{
    MirTouchId slot1 = 0, slot2 = 1, slot3 = 2;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float x = 0;
    float y = 0;
    float orientation = 0;

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(0);
    EXPECT_CALL(mock_sink, handle_input(_)).Times(0);

    touch_screen.start(&mock_sink, &mock_builder);

    //Add 2 touches on coordinate 0,0 - this should be detected as a spurious touch
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot1);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot2, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot2);

    //Add another valid touch event to make sure the frame is really not processed
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot3, 1, 1, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot3);

    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_does_not_ignore_valid_touch_events_on_coordinate_0_0)
{
    MirTouchId slot1 = 0, slot2 = 1;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float x = 0;
    float y = 0;
    float orientation = 0;

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(1);
    EXPECT_CALL(mock_sink, handle_input(_)).Times(1);

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot1);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot2, 1, 1, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_ignores_touch_down_then_touch_up_in_same_frame)
{
    MirTouchId slot2 = 1;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float orientation = 0;

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(0);
    EXPECT_CALL(mock_sink, handle_input(_)).Times(0);

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot2, 1, 1, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_1, slot2);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_with_two_downs_but_one_up)
{
    MirTouchId slot1 = 0;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float x = 1;
    float y = 1;
    float orientation = 0;

    touch_screen.start(&mock_sink, &mock_builder);

    // Send first down
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    // Send second down
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_2, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);

    // Send single up
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_3, slot1);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_3);

    // Detect single touch down, a move, then single up
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(3);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, x, y))).Times(1);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_change, x, y))).Times(1);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(x, y))).Times(1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, ignore_second_down_in_same_frame_then_detect_up)
{
    MirTouchId slot1 = 0;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float x = 1;
    float y = 1;
    float orientation = 0;

    touch_screen.start(&mock_sink, &mock_builder);

    // Send first down
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);

    // Send second down
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    // Send single up
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_2, slot1);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);

    // Detect single touch down, then single up
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(2);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, x, y))).Times(1);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(x, y))).Times(1);

    process_events(touch_screen);
}


TEST_F(LibInputDeviceOnTouchScreen, process_event_spurious_frame_when_down)
{
    MirTouchId slot1 = 0, slot2 = 1;
    float major = 0;
    float minor = 0;
    float pressure = 0.0f;
    float x = 1;
    float y = 1;
    float orientation = 0;

    touch_screen.start(&mock_sink, &mock_builder);

    // Send first down
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot1, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    // Send spurious down+up
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_2, slot1, 0, 0, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_2, slot1);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_2, slot2, 0, 0, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_2, slot2);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);

    // Detect single touch down, a move then single up
    EXPECT_CALL(mock_builder, touch_event(_, _)).Times(2);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, x, y))).Times(1);
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(0, 0))).Times(1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_down_events)
{
    MirTouchId slot = 0;
    float major = 8;
    float minor = 6;
    float orientation = 0;
    float pressure = 0.8f;
    float x = 100;
    float y = 7;

    std::vector<mev::TouchContact> contacts{
        {slot, mir_touch_action_down, mir_touch_tooltype_finger, {x, y}, pressure,
            major, minor, orientation}};

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event({time_stamp_1}, contacts));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x, y)));

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_move_events)
{
    MirTouchId slot = 0;
    float major = 8;
    float minor = 6;
    float pressure = 0.8f;
    float orientation = 0;
    float x = 100;
    float y = 7;

    std::vector<mev::TouchContact> contacts{
        {slot, mir_touch_action_change, mir_touch_tooltype_finger, {x, y},
         pressure, major, minor, orientation}};

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event({time_stamp_1}, contacts));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchMovementEvent()));

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_MOTION, event_time_1, slot, x, y, major,
                                        minor, pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_up_events_without_querying_properties)
{
    MirTouchId slot = 3;
    float major = 8;
    float minor = 6;
    float pressure = 0.8f;
    float x = 30;
    float y = 20;
    float orientation = 0;
    std::vector<mev::TouchContact> contact_down{{slot, mir_touch_action_down, mir_touch_tooltype_finger, {x, y},
        pressure, major, minor, orientation}};
    std::vector<mev::TouchContact> contact_up{{slot, mir_touch_action_up, mir_touch_tooltype_finger, {x, y},
        pressure, major, minor, orientation}};


    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event({time_stamp_1}, contact_down));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x, y)));

    EXPECT_CALL(mock_builder, touch_event({time_stamp_2}, contact_up));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(x, y)));

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor,
                                        pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    env.mock_libinput.setup_touch_up_event(fake_device, event_time_2, slot);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);
    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, sends_complete_events)
{
    const int first_slot = 1;
    const int second_slot = 3;
    const float major = 6;
    const float minor = 5;
    const float pressure = 0.6f;
    const float first_x = 30;
    const float first_y = 20;
    const float second_x = 90;
    const float second_y = 90;
    const float orientation = 0;

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, first_x, first_y)));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y),
                                              mt::TouchContact(1, mir_touch_action_down, second_x, second_y))));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y + 5),
                                              mt::TouchContact(1, mir_touch_action_change, second_x, second_y))));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y + 5),
                                              mt::TouchContact(1, mir_touch_action_change, second_x + 5, second_y))));

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, first_slot, first_x,
                                        first_y, major, minor, pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, second_slot, second_x,
                                        second_y, major, minor, pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_MOTION, event_time_2, first_slot, first_x,
                                        first_y + 5, major, minor, pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_MOTION, event_time_2, second_slot,
                                        second_x + 5, second_y, major, minor, pressure, orientation);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_2);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnLaptopKeyboard, provides_no_pointer_settings_for_non_pointing_devices)
{
    auto settings = keyboard.get_pointer_settings();
    EXPECT_THAT(settings.is_set(), Eq(false));
}

TEST_F(LibInputDeviceOnMouse, reads_pointer_settings_from_libinput)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_none);
    auto optional_settings = mouse.get_pointer_settings();

    EXPECT_THAT(optional_settings.is_set(), Eq(true));

    auto ptr_settings = optional_settings.value();
    EXPECT_THAT(ptr_settings.handedness, Eq(mir_pointer_handedness_right));
    EXPECT_THAT(ptr_settings.cursor_acceleration_bias, Eq(1.0));
    EXPECT_THAT(ptr_settings.horizontal_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.vertical_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.acceleration, Eq(mir_pointer_acceleration_none));

    setup_pointer_configuration(mouse.device(), 0.0, mir_pointer_handedness_left, mir_pointer_acceleration_adaptive);
    optional_settings = mouse.get_pointer_settings();

    EXPECT_THAT(optional_settings.is_set(), Eq(true));

    ptr_settings = optional_settings.value();
    EXPECT_THAT(ptr_settings.handedness, Eq(mir_pointer_handedness_left));
    EXPECT_THAT(ptr_settings.cursor_acceleration_bias, Eq(0.0));
    EXPECT_THAT(ptr_settings.horizontal_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.vertical_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.acceleration, Eq(mir_pointer_acceleration_adaptive));
}

TEST_F(LibInputDeviceOnMouse, applies_pointer_settings)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_adaptive);
    mi::PointerSettings settings(mouse.get_pointer_settings().value());
    settings.cursor_acceleration_bias = 1.1;
    settings.handedness = mir_pointer_handedness_left;
    settings.acceleration = mir_pointer_acceleration_none;

    EXPECT_CALL(env.mock_libinput, libinput_device_config_accel_set_speed(mouse.device(), 1.1)).Times(1);
    EXPECT_CALL(env.mock_libinput, libinput_device_config_accel_set_profile(mouse.device(), LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)).Times(1);
    EXPECT_CALL(env.mock_libinput, libinput_device_config_left_handed_set(mouse.device(), true)).Times(1);

    mouse.apply_settings(settings);
}

TEST_F(LibInputDeviceOnLaptopKeyboardAndMouse, denies_pointer_settings_on_keyboards)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_adaptive);
    auto settings_from_mouse = mouse.get_pointer_settings();

    EXPECT_CALL(env.mock_libinput,libinput_device_config_accel_set_speed(_, _)).Times(0);
    EXPECT_CALL(env.mock_libinput,libinput_device_config_left_handed_set(_, _)).Times(0);

    keyboard.apply_settings(settings_from_mouse.value());
}

TEST_F(LibInputDeviceOnMouse, scroll_speed_scales_scroll_events)
{
    // expect two scroll events..
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 3.0f)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, -10.0f)));

    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_none);
    mi::PointerSettings settings(mouse.get_pointer_settings().value());
    settings.vertical_scroll_scale = -1.0;
    settings.horizontal_scroll_scale = 5.0;
    mouse.apply_settings(settings);

    env.mock_libinput.setup_axis_event(fake_device, event_time_1, {}, -3.0);
    env.mock_libinput.setup_axis_event(fake_device, event_time_2, -2.0, {});

    mouse.start(&mock_sink, &mock_builder);
    process_events(mouse);
}

TEST_F(LibInputDeviceOnLaptopKeyboardAndMouse, provides_no_touchpad_settings_for_non_touchpad_devices)
{
    auto val = keyboard.get_touchpad_settings();
    EXPECT_THAT(val.is_set(), Eq(false));
    val = mouse.get_touchpad_settings();
    EXPECT_THAT(val.is_set(), Eq(false));
}

TEST_F(LibInputDeviceOnTouchpad, process_event_handles_scroll)
{
    InSequence seq;
    // expect two scroll events..
    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_1}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_finger,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{geom::DeltaYF{-10}, {}, false}));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, -10.0f)));
    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_2}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_finger,
        mev::ScrollAxisH{geom::DeltaXF{1}, {}, false},
        mev::ScrollAxisV{}));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 1.0f)));

    env.mock_libinput.setup_finger_axis_event(fake_device, event_time_1, {}, -10.0);
    env.mock_libinput.setup_finger_axis_event(fake_device, event_time_2, 1.0, {});
    touchpad.start(&mock_sink, &mock_builder);
    process_events(touchpad);
}

TEST_F(LibInputDeviceOnTouchpad, process_event_handles_stop)
{
    InSequence seq;
    // expect two scroll events..
    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_1}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_finger,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{geom::DeltaYF{-10}, {}, false}));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, -10.0f)));
    EXPECT_CALL(mock_builder, pointer_event(
        {time_stamp_2}, mir_pointer_action_motion, 0,
        Eq(std::nullopt), geom::DisplacementF{},
        mir_pointer_axis_source_finger,
        mev::ScrollAxisH{},
        mev::ScrollAxisV{{}, {}, true}));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 0.0f)));

    env.mock_libinput.setup_finger_axis_event(fake_device, event_time_1, {}, -10.0);
    env.mock_libinput.setup_finger_axis_event(fake_device, event_time_2, {}, 0.0);
    touchpad.start(&mock_sink, &mock_builder);
    process_events(touchpad);
}

TEST_F(LibInputDeviceOnTouchpad, reads_touchpad_settings_from_libinput)
{
    setup_touchpad_configuration(fake_device, mir_touchpad_click_mode_finger_count,
                                  mir_touchpad_scroll_mode_edge_scroll, 0, true, false, true, false);

    auto settings = touchpad.get_touchpad_settings().value();
    EXPECT_THAT(settings.click_mode, Eq(mir_touchpad_click_mode_finger_count));
    EXPECT_THAT(settings.scroll_mode, Eq(mir_touchpad_scroll_mode_edge_scroll));
    EXPECT_THAT(settings.tap_to_click, Eq(true));
    EXPECT_THAT(settings.disable_while_typing, Eq(false));
    EXPECT_THAT(settings.disable_with_external_mouse, Eq(true));
    EXPECT_THAT(settings.middle_mouse_button_emulation, Eq(false));
}

TEST_F(LibInputDeviceOnTouchpad, applies_touchpad_settings)
{
    setup_touchpad_configuration(fake_device, mir_touchpad_click_mode_finger_count,
                                  mir_touchpad_scroll_mode_two_finger_scroll, 0, true, false, true, false);

    mi::TouchpadSettings settings(touchpad.get_touchpad_settings().value());
    settings.scroll_mode = mir_touchpad_scroll_mode_button_down_scroll;
    settings.click_mode = mir_touchpad_click_mode_finger_count;
    settings.button_down_scroll_button = KEY_A;
    settings.tap_to_click = true;
    settings.disable_while_typing = false;
    settings.disable_with_external_mouse = true;
    settings.middle_mouse_button_emulation = true;

    EXPECT_CALL(env.mock_libinput,
                libinput_device_config_scroll_set_method(touchpad.device(), LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN));
    EXPECT_CALL(env.mock_libinput,
                libinput_device_config_click_set_method(touchpad.device(), LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER));
    EXPECT_CALL(env.mock_libinput, libinput_device_config_scroll_set_button(touchpad.device(), KEY_A));
    EXPECT_CALL(env.mock_libinput,
                libinput_device_config_tap_set_enabled(touchpad.device(), LIBINPUT_CONFIG_TAP_ENABLED));
    EXPECT_CALL(env.mock_libinput,
                libinput_device_config_dwt_set_enabled(touchpad.device(), LIBINPUT_CONFIG_DWT_DISABLED));
    EXPECT_CALL(env.mock_libinput, libinput_device_config_send_events_set_mode(
                                       touchpad.device(), LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE));
    EXPECT_CALL(env.mock_libinput, libinput_device_config_middle_emulation_set_enabled(
                                       touchpad.device(), LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED));

    touchpad.apply_settings(settings);
}

TEST_F(LibInputDevice, device_ptr_keeps_libinput_context_alive)
{
    auto fake_dev = setup_touchpad();

    InSequence seq;
    EXPECT_CALL(env.mock_libinput, libinput_device_ref(fake_dev));
    EXPECT_CALL(env.mock_libinput, libinput_device_unref(fake_dev));
    EXPECT_CALL(env.mock_libinput, libinput_unref(env.li_context));

    auto device_ptr = mie::make_libinput_device(lib, fake_dev);

    lib.reset();
    device_ptr.reset();
}

TEST_F(LibInputDeviceOnTouchScreen, device_maps_to_selected_output)
{
    const float x = 30;
    const float y = 20;
    const uint32_t output_id = 2;
    const float output_x_pos = 20;
    const float output_y_pos = 50;
    const int width = 100;
    const int height = 200;

    EXPECT_CALL(mock_sink, bounding_rectangle())
        .Times(0);
    EXPECT_CALL(mock_sink, output_info(output_id))
        .WillRepeatedly(Return(
                mi::OutputInfo{
                    true,
                    geom::Size{width, height},
                    Matrix{{1.0f, 0.0f, output_x_pos,
                            0.0f, 1.0f, output_y_pos}}}));
    EXPECT_CALL(mock_sink,
                handle_input(
                    mt::TouchContact(
                        0,
                        mir_touch_action_down,
                        x + output_x_pos,
                        y + output_y_pos)))
        .Times(1);

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_output});
    touch_screen.start(&mock_sink, &mock_builder);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, device_maps_to_left_rotated_output)
{
    const float x = 10;
    const float y = 5;
    const uint32_t output_id = 2;
    const float output_x_pos = 20;
    const float output_y_pos = 50;
    const int width = 100;
    const int height = 200;

    EXPECT_CALL(mock_sink, bounding_rectangle())
        .Times(0);
    EXPECT_CALL(mock_sink, output_info(output_id))
        .WillRepeatedly(Return(
                mi::OutputInfo{
                               true,
                               geom::Size{width, height},
                               Matrix{{0.0f, 1.0f, output_x_pos,
                                       -1.0f, 0.0f, width + output_y_pos}}}));
    EXPECT_CALL(mock_sink,
                handle_input(
                    mt::TouchContact(
                        0,
                        mir_touch_action_down,
                        output_x_pos + y,
                        output_y_pos + width - x)))
        .Times(1);

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_output});
    touch_screen.start(&mock_sink, &mock_builder);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, device_maps_to_right_rotated_output)
{
    EXPECT_CALL(mock_sink, bounding_rectangle())
        .Times(0);
    EXPECT_CALL(mock_sink, output_info(output_id))
        .WillRepeatedly(Return(
                mi::OutputInfo{
                    true,
                    geom::Size{width, height},
                    Matrix{{0.0f, -1.0f, height + output_x_pos,
                            1.0f, 0.0f, output_y_pos}}}));
    EXPECT_CALL(mock_sink,
                handle_input(
                    mt::TouchContact(
                        0,
                        mir_touch_action_down,
                        output_x_pos + height - y,
                        output_y_pos + x)))
        .Times(1);

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_output});
    touch_screen.start(&mock_sink, &mock_builder);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, device_maps_to_inverted_output)
{
    EXPECT_CALL(mock_sink, bounding_rectangle())
        .Times(0);
    EXPECT_CALL(mock_sink, output_info(output_id))
        .WillRepeatedly(Return(
                mi::OutputInfo{
                    true,
                    geom::Size{width, height},
                    Matrix{{-1.0f, 0.0f, width + output_x_pos,
                            0.0f, -1.0f, height + output_y_pos}}}));
    EXPECT_CALL(mock_sink,
                handle_input(
                    mt::TouchContact(
                        0,
                        mir_touch_action_down,
                        output_x_pos + width - x,
                        output_y_pos + height - y)))
        .Times(1);

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_output});
    touch_screen.start(&mock_sink, &mock_builder);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, display_wall_device_maps_to_bounding_rectangle)
{
    EXPECT_CALL(mock_sink, output_info(output_id))
        .Times(0);
    EXPECT_CALL(mock_sink, bounding_rectangle())
        .WillOnce(Return(geom::Rectangle{geom::Point{screen_x_pos, screen_y_pos}, geom::Size{100, 100}}));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, x + screen_x_pos, y + screen_y_pos)))
        .Times(1);

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_display_wall});
    touch_screen.start(&mock_sink, &mock_builder);

    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}

TEST_F(LibInputDeviceOnTouchScreen, drops_touchscreen_event_on_deactivated_output)
{
    EXPECT_CALL(mock_sink, handle_input(_)).Times(0);

    ON_CALL(mock_sink, output_info(output_id))
        .WillByDefault(Return(mi::OutputInfo{false, geom::Size{width, height}, Matrix{{1,0,0,0,1,0}}}));

    touch_screen.apply_settings(mi::TouchscreenSettings{output_id, mir_touchscreen_mapping_mode_to_output});

    touch_screen.start(&mock_sink, &mock_builder);
    env.mock_libinput.setup_touch_event(fake_device, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, 0, x, y, 0, 0, 0, 0);
    env.mock_libinput.setup_touch_frame(fake_device, event_time_1);

    process_events(touch_screen);
}
