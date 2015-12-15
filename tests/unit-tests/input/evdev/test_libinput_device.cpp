/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "src/platforms/evdev/libinput_device.h"
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
#include "mir/test/doubles/mock_libinput.h"
#include "mir/test/gmock_fixes.h"
#include "mir/udev/wrapper.h"
#include "mir/cookie_factory.h"
#include "mir_test_framework/udev_environment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>
#include <libinput.h>

#include <chrono>

namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mtd = mt::doubles;
namespace geom = mir::geometry;

namespace
{

class StubInputDeviceRegistry : public mi::InputDeviceRegistry
{
public:
    void add_device(std::shared_ptr<mi::InputDevice> const&) override {}
    void remove_device(std::shared_ptr<mi::InputDevice> const&) override {}
};

using namespace ::testing;

struct MockInputSink : mi::InputSink
{
    MockInputSink()
    {
        ON_CALL(*this, bounding_rectangle())
            .WillByDefault(Return(geom::Rectangle({0,0}, {100,100})));
    }
    MOCK_METHOD1(handle_input,void(MirEvent &));
    MOCK_METHOD1(confine_pointer, void(geom::Point&));
    MOCK_CONST_METHOD0(bounding_rectangle, geom::Rectangle());
};

struct MockEventBuilder : mi::EventBuilder
{
    std::shared_ptr<mir::cookie::CookieFactory> const cookie_factory = mir::cookie::CookieFactory::create_keeping_secret();
    mi::DefaultEventBuilder builder{MirInputDeviceId{3}, cookie_factory};
    MockEventBuilder()
    {
        ON_CALL(*this, key_event(_,_,_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirKeyboardAction action, xkb_keysym_t key, int scan_code)
                                  {
                                        return builder.key_event(time, action, key, scan_code);
                                  }));
        ON_CALL(*this, touch_event(_))
            .WillByDefault(Invoke([this](Timestamp time)
                                  {
                                        return builder.touch_event(time);
                                  }));
        ON_CALL(*this, add_touch(_,_,_,_,_,_,_,_,_,_))
            .WillByDefault(Invoke([this](MirEvent& event, MirTouchId id, MirTouchAction action,
                                         MirTouchTooltype tooltype, float x, float y, float major, float minor,
                                         float pressure, float size)
                                  {
                                        return builder.add_touch(event, id, action, tooltype, x, y, major, minor,
                                                                 pressure, size);
                                  }));
        ON_CALL(*this, pointer_event(_, _, _, _, _, _, _))
            .WillByDefault(Invoke([this](Timestamp time, MirPointerAction action, MirPointerButtons buttons,
                                         float hscroll, float vscroll, float relative_x, float relative_y)
                                  {
                                      return builder.pointer_event(time, action, buttons, hscroll, vscroll,
                                                                   relative_x, relative_y);
                                  }));
        ON_CALL(*this, configuration_event(_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirInputConfigurationAction action)
                                  {
                                        return builder.configuration_event(time, action);
                                  }));
    }
    using EventBuilder::Timestamp;
    MOCK_METHOD4(key_event, mir::EventUPtr(Timestamp, MirKeyboardAction, xkb_keysym_t, int));

    MOCK_METHOD1(touch_event, mir::EventUPtr(Timestamp));
    MOCK_METHOD10(add_touch, void(MirEvent&, MirTouchId, MirTouchAction, MirTouchTooltype, float, float, float, float,
                                  float, float));
    MOCK_METHOD7(pointer_event,
                 mir::EventUPtr(Timestamp, MirPointerAction, MirPointerButtons, float, float, float, float));
    MOCK_METHOD2(configuration_event, mir::EventUPtr(Timestamp, MirInputConfigurationAction));
};

struct LibInputDevice : public ::testing::Test
{
    mtf::UdevEnvironment env;
    ::testing::NiceMock<mir::test::doubles::MockLibInput> mock_libinput;
    ::testing::NiceMock<MockInputSink> mock_sink;
    ::testing::NiceMock<MockEventBuilder> mock_builder;
    std::shared_ptr<libinput> lib;

    libinput* fake_input = reinterpret_cast<libinput*>(0xF4C3);
    libinput_device* fake_device = reinterpret_cast<libinput_device*>(0xF4C4);
    libinput_event* fake_event_1 = reinterpret_cast<libinput_event*>(0xF4C5);
    libinput_event* fake_event_2 = reinterpret_cast<libinput_event*>(0xF4C6);
    libinput_event* fake_event_3 = reinterpret_cast<libinput_event*>(0xF4C7);
    libinput_event* fake_event_4 = reinterpret_cast<libinput_event*>(0xF4C8);
    libinput_event* fake_event_5 = reinterpret_cast<libinput_event*>(0xF4D0);
    libinput_event* fake_event_6 = reinterpret_cast<libinput_event*>(0xF4D1);
    libinput_event* fake_event_7 = reinterpret_cast<libinput_event*>(0xF4D2);
    libinput_event* fake_event_8 = reinterpret_cast<libinput_event*>(0xF4D3);
    libinput_device* second_fake_device = reinterpret_cast<libinput_device*>(0xF4C9);

    const uint64_t event_time_1 = 1000;
    const mi::EventBuilder::Timestamp time_stamp_1{std::chrono::microseconds{event_time_1}};
    const uint64_t event_time_2 = 2000;
    const mi::EventBuilder::Timestamp time_stamp_2{std::chrono::microseconds{event_time_2}};
    const uint64_t event_time_3 = 3000;
    const mi::EventBuilder::Timestamp time_stamp_3{std::chrono::microseconds{event_time_3}};
    const uint64_t event_time_4 = 4000;
    const mi::EventBuilder::Timestamp time_stamp_4{std::chrono::microseconds{event_time_4}};

    char const* laptop_keyboard_device_path = "/dev/input/event4";
    char const* trackpad_dev_path = "/dev/input/event13";
    char const* touch_screen_dev_path = "/dev/input/event4";
    char const* usb_mouse_dev_path = "/dev/input/event13";
    char const* touchpad_dev_path = "/dev/input/event12";

    LibInputDevice()
    {
        ON_CALL(mock_libinput, libinput_path_create_context(_,_))
            .WillByDefault(Return(fake_input));
        lib = mie::make_libinput();
    }

    char const* setup_laptop_keyboard(libinput_device* dev)
    {
        return setup_device(dev, laptop_keyboard_device_path, "laptop-keyboard", 5252, 3113);
    }

    char const* setup_trackpad(libinput_device* dev)
    {
        return setup_device(dev, trackpad_dev_path, "bluetooth-magic-trackpad", 9663, 1234);
    }

    char const* setup_touch_screen(libinput_device* dev)
    {
        return setup_device(dev, touch_screen_dev_path, "mt-screen-detection", 858, 484);
    }

    char const* setup_touchpad(libinput_device* dev)
    {
        return setup_device(dev, touchpad_dev_path, "synaptics-touchpad", 858, 484);
    }

    char const* setup_mouse(libinput_device* dev)
    {
        return setup_device(dev, usb_mouse_dev_path, "usb-mouse", 858, 484);
    }

    void remove_devices()
    {
        mir::udev::Enumerator devices{std::make_shared<mir::udev::Context>()};
        devices.scan_devices();

        for (auto& device : devices)
        {
            if (device.devnode() && (std::string(device.devnode()).find("input/event") != std::string::npos))
            {
                env.remove_device((std::string("/sys") + device.devpath()).c_str());
            }
        }
    }

    char const* setup_device(libinput_device* dev, char const* device_path, char const* umock_name, unsigned int vendor_id, unsigned int product_id)
    {
        env.add_standard_device(umock_name);
        mock_libinput.setup_device(fake_input, dev, device_path, umock_name, vendor_id, product_id);
        return device_path;
    }

    void setup_pointer_configuration(libinput_device* dev, double accel_speed, MirPointerHandedness handedness, MirPointerAccelerationProfile profile)
    {
        ON_CALL(mock_libinput, libinput_device_config_accel_get_speed(dev))
            .WillByDefault(Return(accel_speed));
        ON_CALL(mock_libinput, libinput_device_config_left_handed_get(dev))
            .WillByDefault(Return(handedness == mir_pointer_handedness_left));
        ON_CALL(mock_libinput, libinput_device_config_accel_get_profile(dev))
            .WillByDefault(Return((profile == mir_pointer_acceleration_profile_flat) ?
                                      LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT :
                                      (profile == mir_pointer_acceleration_profile_none) ?
                                      LIBINPUT_CONFIG_ACCEL_PROFILE_NONE :
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

        ON_CALL(mock_libinput, libinput_device_config_click_get_method(dev))
            .WillByDefault(Return(static_cast<libinput_config_click_method>(click_method.value())));
        ON_CALL(mock_libinput, libinput_device_config_scroll_get_method(dev))
            .WillByDefault(Return(static_cast<libinput_config_scroll_method>(scroll_method.value())));
        ON_CALL(mock_libinput, libinput_device_config_scroll_get_button(dev))
            .WillByDefault(Return(scroll_button));
        ON_CALL(mock_libinput, libinput_device_config_tap_get_enabled(dev))
            .WillByDefault(Return(tap_to_click?
                                   LIBINPUT_CONFIG_TAP_ENABLED:
                                   LIBINPUT_CONFIG_TAP_DISABLED));
        ON_CALL(mock_libinput, libinput_device_config_dwt_get_enabled(dev))
            .WillByDefault(Return(disable_while_typing?
                                   LIBINPUT_CONFIG_DWT_ENABLED:
                                   LIBINPUT_CONFIG_DWT_DISABLED));
        ON_CALL(mock_libinput, libinput_device_config_send_events_get_mode(dev))
            .WillByDefault(Return(disable_with_mouse?
                                   LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE:
                                   LIBINPUT_CONFIG_SEND_EVENTS_ENABLED));
        ON_CALL(mock_libinput, libinput_device_config_middle_emulation_get_enabled(dev))
            .WillByDefault(Return(middle_button_emulation?
                                   LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED:
                                   LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED));

    }

    void setup_key_event(libinput_event* event, uint64_t event_time, uint32_t key, libinput_key_state state)
    {
        auto key_event = reinterpret_cast<libinput_event_keyboard*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_KEYBOARD_KEY));
        ON_CALL(mock_libinput, libinput_event_get_keyboard_event(event))
            .WillByDefault(Return(key_event));
        ON_CALL(mock_libinput, libinput_event_keyboard_get_time_usec(key_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_keyboard_get_key(key_event))
            .WillByDefault(Return(key));
        ON_CALL(mock_libinput, libinput_event_keyboard_get_key_state(key_event))
            .WillByDefault(Return(state));
    }

    void setup_pointer_event(libinput_event* event, uint64_t event_time, float relatve_x, float relatve_y)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_POINTER_MOTION));
        ON_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillByDefault(Return(pointer_event));
        ON_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_pointer_get_dx(pointer_event))
            .WillByDefault(Return(relatve_x));
        ON_CALL(mock_libinput, libinput_event_pointer_get_dy(pointer_event))
            .WillByDefault(Return(relatve_y));
    }

    void setup_absolute_pointer_event(libinput_event* event, uint64_t event_time, float x, float y)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE));
        ON_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillByDefault(Return(pointer_event));
        ON_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_pointer_get_absolute_x_transformed(pointer_event, _))
            .WillByDefault(Return(x));
        ON_CALL(mock_libinput, libinput_event_pointer_get_absolute_y_transformed(pointer_event, _))
            .WillByDefault(Return(y));
    }

    void setup_button_event(libinput_event* event, uint64_t event_time, int button, libinput_button_state state)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_POINTER_BUTTON));
        ON_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillByDefault(Return(pointer_event));
        ON_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_pointer_get_button(pointer_event))
            .WillByDefault(Return(button));
        ON_CALL(mock_libinput, libinput_event_pointer_get_button_state(pointer_event))
            .WillByDefault(Return(state));
    }

    void setup_axis_event(libinput_event* event, uint64_t event_time, double horizontal, double vertical)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_POINTER_AXIS));
        ON_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillByDefault(Return(pointer_event));
        ON_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_source(pointer_event))
            .WillByDefault(Return(LIBINPUT_POINTER_AXIS_SOURCE_WHEEL));
        ON_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillByDefault(Return(horizontal!=0.0));
        ON_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillByDefault(Return(vertical!=0.0));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_value_discrete(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillByDefault(Return(vertical));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_value_discrete(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillByDefault(Return(horizontal));
    }

    void setup_finger_axis_event(libinput_event* event, uint64_t event_time, double horizontal, double vertical)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_POINTER_AXIS));
        ON_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillByDefault(Return(pointer_event));
        ON_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_source(pointer_event))
            .WillByDefault(Return(LIBINPUT_POINTER_AXIS_SOURCE_FINGER));
        ON_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillByDefault(Return(horizontal!=0.0));
        ON_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillByDefault(Return(vertical!=0.0));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillByDefault(Return(vertical));
        ON_CALL(mock_libinput, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillByDefault(Return(horizontal));
    }

    void setup_touch_event(libinput_event* event, libinput_event_type type, uint64_t event_time, int slot, float x,
                           float y, float major, float minor, float pressure)
    {
        auto touch_event = reinterpret_cast<libinput_event_touch*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(type));
        ON_CALL(mock_libinput, libinput_event_get_touch_event(event))
            .WillByDefault(Return(touch_event));
        ON_CALL(mock_libinput, libinput_event_touch_get_slot(touch_event))
            .WillByDefault(Return(slot));
        ON_CALL(mock_libinput, libinput_event_touch_get_x_transformed(touch_event, _))
            .WillByDefault(Return(x));
        ON_CALL(mock_libinput, libinput_event_touch_get_y_transformed(touch_event, _))
            .WillByDefault(Return(y));
        ON_CALL(mock_libinput, libinput_event_touch_get_time_usec(touch_event))
            .WillByDefault(Return(event_time));
        ON_CALL(mock_libinput, libinput_event_touch_get_major_transformed(touch_event, _, _))
            .WillByDefault(Return(major));
        ON_CALL(mock_libinput, libinput_event_touch_get_minor_transformed(touch_event, _, _))
            .WillByDefault(Return(minor));
        ON_CALL(mock_libinput, libinput_event_touch_get_pressure(touch_event))
            .WillByDefault(Return(pressure));
    }

    void setup_touch_up_event(libinput_event* event, uint64_t event_time, int slot)
    {
        auto touch_event = reinterpret_cast<libinput_event_touch*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_TOUCH_UP));
        ON_CALL(mock_libinput, libinput_event_get_touch_event(event))
            .WillByDefault(Return(touch_event));
        ON_CALL(mock_libinput, libinput_event_touch_get_slot(touch_event))
            .WillByDefault(Return(slot));
        ON_CALL(mock_libinput, libinput_event_touch_get_time_usec(touch_event))
            .WillByDefault(Return(event_time));
    }

    void setup_touch_frame(libinput_event* event, uint64_t event_time)
    {
        auto touch_event = reinterpret_cast<libinput_event_touch*>(event);

        ON_CALL(mock_libinput, libinput_event_get_type(event))
            .WillByDefault(Return(LIBINPUT_EVENT_TOUCH_FRAME));
        ON_CALL(mock_libinput, libinput_event_get_touch_event(event))
            .WillByDefault(Return(touch_event));
        ON_CALL(mock_libinput, libinput_event_touch_get_time_usec(touch_event))
            .WillByDefault(Return(event_time));
    }
};

struct LibInputDeviceOnLaptopKeyboard : public LibInputDevice
{
    char const* keyboard_path = setup_laptop_keyboard(fake_device);
    mie::LibInputDevice keyboard{mir::report::null_input_report(), keyboard_path, mie::make_libinput_device(lib, keyboard_path)};
};

struct LibInputDeviceOnMouse : public LibInputDevice
{
    char const* mouse_path = setup_mouse(fake_device);
    mie::LibInputDevice mouse{mir::report::null_input_report(), mouse_path, mie::make_libinput_device(lib, mouse_path)};
};

struct LibInputDeviceOnLaptopKeyboardAndMouse : public LibInputDevice
{
    char const* mouse_path = setup_mouse(fake_device);
    char const* keyboard_path = setup_laptop_keyboard(fake_device);
    mie::LibInputDevice keyboard{mir::report::null_input_report(), keyboard_path, mie::make_libinput_device(lib, keyboard_path)};
    mie::LibInputDevice mouse{mir::report::null_input_report(), mouse_path, mie::make_libinput_device(lib, mouse_path)};
};

struct LibInputDeviceOnTouchScreen : public LibInputDevice
{
    char const* touch_screen_path = setup_touch_screen(fake_device);
    mie::LibInputDevice touch_screen{mir::report::null_input_report(), touch_screen_path, mie::make_libinput_device(lib, touch_screen_path)};
};

struct LibInputDeviceOnTouchpad : public LibInputDevice
{
    char const* touchpad_path = setup_touchpad(fake_device);
    mie::LibInputDevice touchpad{mir::report::null_input_report(), touchpad_path, mie::make_libinput_device(lib, touchpad_path)};
};
}

TEST_F(LibInputDevice, start_creates_and_unrefs_libinput_device_from_path)
{
    char const * path = setup_laptop_keyboard(fake_device);

    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(path)))
        .Times(1);
    // according to manual libinput_path_add_device creates a temporary device with a ref count 0.
    // hence it needs a manual ref call
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            path,
                            std::move(mie::make_libinput_device(lib, path)));
    dev.start(&mock_sink, &mock_builder);
}

TEST_F(LibInputDevice, open_device_of_group)
{
    char const* first_path = setup_laptop_keyboard(fake_device);
    char const* second_path = setup_trackpad(second_fake_device);

    InSequence seq;
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(first_path))).Times(1);
    // according to manual libinput_path_add_device creates a temporary device with a ref count 0.
    // hence it needs a manual ref call
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device)).Times(1);
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(second_path))).Times(1);
    EXPECT_CALL(mock_libinput, libinput_device_ref(second_fake_device)).Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            first_path,
                            std::move(mie::make_libinput_device(lib, first_path)));
    dev.add_device_of_group(second_path, mie::make_libinput_device(lib, second_path));
    dev.start(&mock_sink, &mock_builder);
}

TEST_F(LibInputDevice, input_info_combines_capabilities)
{
    char const* first_dev = setup_laptop_keyboard(fake_device);
    char const* second_dev = setup_trackpad(second_fake_device);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            first_dev,
                            mie::make_libinput_device(lib, first_dev));
    dev.add_device_of_group(second_dev, mie::make_libinput_device(lib, second_dev));
    auto info = dev.get_device_info();

    EXPECT_THAT(info.capabilities, Eq(mi::DeviceCapability::touchpad |
                                      mi::DeviceCapability::pointer |
                                      mi::DeviceCapability::keyboard |
                                      mi::DeviceCapability::alpha_numeric));
}

TEST_F(LibInputDevice, removal_unrefs_libinput_device)
{
    char const* path = setup_laptop_keyboard(fake_device);

    EXPECT_CALL(mock_libinput, libinput_device_unref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib, path));
}

TEST_F(LibInputDeviceOnLaptopKeyboard, process_event_converts_key_event)
{
    setup_key_event(fake_event_1, event_time_1, KEY_A, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_2, event_time_2, KEY_A, LIBINPUT_KEY_STATE_RELEASED);

    EXPECT_CALL(mock_builder, key_event(time_stamp_1, mir_keyboard_action_down, _, KEY_A));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_2, mir_keyboard_action_up, _, KEY_A));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyUpEvent())));

    keyboard.start(&mock_sink, &mock_builder);
    keyboard.process_event(fake_event_1);
    keyboard.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnLaptopKeyboard, process_event_accumulates_key_state)
{
    setup_key_event(fake_event_1, event_time_1, KEY_C, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_2, event_time_2, KEY_LEFTALT, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_3, event_time_3, KEY_C, LIBINPUT_KEY_STATE_RELEASED);

    InSequence seq;
    EXPECT_CALL(mock_builder, key_event(time_stamp_1, mir_keyboard_action_down, _, KEY_C));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_2, mir_keyboard_action_down, _, KEY_LEFTALT));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_LEFTALT),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_3, mir_keyboard_action_up, _, KEY_C));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),
                                              mt::KeyUpEvent())));

    keyboard.start(&mock_sink, &mock_builder);
    keyboard.process_event(fake_event_1);
    keyboard.process_event(fake_event_2);
    keyboard.process_event(fake_event_3);
}

TEST_F(LibInputDeviceOnMouse, process_event_converts_pointer_event)
{
    float x_movement_1 = 15;
    float y_movement_1 = 17;
    float x_movement_2 = 20;
    float y_movement_2 = 40;
    setup_pointer_event(fake_event_1, event_time_1, x_movement_1, y_movement_1);
    setup_pointer_event(fake_event_2, event_time_2, x_movement_2, y_movement_2);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x_movement_1,y_movement_1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x_movement_2,y_movement_2)));

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_absolute_pointer_events)
{
    float x1 = 15;
    float y1 = 17;
    float x2 = 40;
    float y2 = 10;
    setup_absolute_pointer_event(fake_event_1, event_time_1, x1, y1);
    setup_absolute_pointer_event(fake_event_2, event_time_2, x2, y2);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x1, y1)));
    EXPECT_CALL(mock_sink,
                handle_input(mt::PointerEventWithDiff(x2 - x1, y2 - y1)));

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnMouse, process_event_motion_events_with_relative_changes)
{
    float x1 = 15, x2 = 23;
    float y1 = 17, y2 = 21;

    setup_pointer_event(fake_event_1, event_time_1, x1, y1);
    setup_pointer_event(fake_event_2, event_time_2, x2, y2);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x1,y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x2,y2)));

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_press_and_release)
{
    float const x = 0;
    float const y = 0;
    geom::Point const pos{x, y};

    setup_button_event(fake_event_1, event_time_1, BTN_LEFT, LIBINPUT_BUTTON_STATE_PRESSED);
    setup_button_event(fake_event_2, event_time_2, BTN_RIGHT, LIBINPUT_BUTTON_STATE_PRESSED);
    setup_button_event(fake_event_3, event_time_3, BTN_RIGHT, LIBINPUT_BUTTON_STATE_RELEASED);
    setup_button_event(fake_event_4, event_time_4, BTN_LEFT, LIBINPUT_BUTTON_STATE_RELEASED);

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_primary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_primary)));

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
    mouse.process_event(fake_event_3);
    mouse.process_event(fake_event_4);
}

TEST_F(LibInputDeviceOnMouse, process_event_handles_scroll)
{
    setup_axis_event(fake_event_1, event_time_1, 0.0, 20.0);
    setup_axis_event(fake_event_2, event_time_2, 5.0, 0.0);

    InSequence seq;
    // expect two scroll events..
    EXPECT_CALL(mock_builder,
                pointer_event(time_stamp_1, mir_pointer_action_motion, 0, 0.0f, -20.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, -20.0f)));
    EXPECT_CALL(mock_builder,
                pointer_event(time_stamp_2, mir_pointer_action_motion, 0, 5.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 5.0f)));

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_down_events)
{
    int slot = 0;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 100;
    float y = 7;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2, event_time_1);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{0}, mir_touch_action_down, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x, y)));

    touch_screen.start(&mock_sink, &mock_builder);
    touch_screen.process_event(fake_event_1);
    touch_screen.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_move_events)
{
    int slot = 0;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 100;
    float y = 7;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_MOTION, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2, event_time_1);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{0}, mir_touch_action_change, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchMovementEvent()));

    touch_screen.start(&mock_sink, &mock_builder);
    touch_screen.process_event(fake_event_1);
    touch_screen.process_event(fake_event_2);
}

TEST_F(LibInputDeviceOnTouchScreen, process_event_handles_touch_up_events_without_querying_properties)
{
    int slot = 3;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 30;
    float y = 20;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2, event_time_1);
    setup_touch_up_event(fake_event_3, event_time_2, slot);
    setup_touch_frame(fake_event_4, event_time_2);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{slot}, mir_touch_action_down, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x, y)));

    EXPECT_CALL(mock_builder, touch_event(time_stamp_2));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{slot}, mir_touch_action_up, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(x, y)));

    touch_screen.start(&mock_sink, &mock_builder);
    touch_screen.process_event(fake_event_1);
    touch_screen.process_event(fake_event_2);
    touch_screen.process_event(fake_event_3);
    touch_screen.process_event(fake_event_4);
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

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, first_slot, first_x, first_y, major, minor, pressure);
    setup_touch_frame(fake_event_2, event_time_1);
    setup_touch_event(fake_event_3, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, second_slot, second_x, second_y, major, minor, pressure);
    setup_touch_frame(fake_event_4, event_time_1);
    setup_touch_event(fake_event_5, LIBINPUT_EVENT_TOUCH_MOTION, event_time_2, first_slot, first_x, first_y + 5, major, minor, pressure);
    setup_touch_frame(fake_event_6, event_time_2);
    setup_touch_event(fake_event_7, LIBINPUT_EVENT_TOUCH_MOTION, event_time_2, second_slot, second_x + 5, second_y, major, minor, pressure);
    setup_touch_frame(fake_event_8, event_time_2);

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::TouchContact(0, mir_touch_action_down, first_x, first_y)));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y),
                                              mt::TouchContact(1, mir_touch_action_down, second_x, second_y))));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y + 5),
                                              mt::TouchContact(1, mir_touch_action_change, second_x, second_y))));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::TouchContact(0, mir_touch_action_change, first_x, first_y + 5),
                                              mt::TouchContact(1, mir_touch_action_change, second_x + 5, second_y))));


    touch_screen.start(&mock_sink, &mock_builder);
    touch_screen.process_event(fake_event_1);
    touch_screen.process_event(fake_event_2);
    touch_screen.process_event(fake_event_3);
    touch_screen.process_event(fake_event_4);
    touch_screen.process_event(fake_event_5);
    touch_screen.process_event(fake_event_6);
    touch_screen.process_event(fake_event_7);
    touch_screen.process_event(fake_event_8);
}

TEST_F(LibInputDeviceOnLaptopKeyboard, provides_no_pointer_settings_for_non_pointing_devices)
{
    auto settings = keyboard.get_pointer_settings();
    EXPECT_THAT(settings.is_set(), Eq(false));
}

TEST_F(LibInputDeviceOnMouse, reads_pointer_settings_from_libinput)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_profile_flat);
    auto optional_settings = mouse.get_pointer_settings();

    EXPECT_THAT(optional_settings.is_set(), Eq(true));

    auto ptr_settings = optional_settings.value();
    EXPECT_THAT(ptr_settings.handedness, Eq(mir_pointer_handedness_right));
    EXPECT_THAT(ptr_settings.cursor_acceleration_bias, Eq(1.0));
    EXPECT_THAT(ptr_settings.horizontal_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.vertical_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.acceleration_profile, Eq(mir_pointer_acceleration_profile_flat));

    setup_pointer_configuration(mouse.device(), 0.0, mir_pointer_handedness_left, mir_pointer_acceleration_profile_adaptive);
    optional_settings = mouse.get_pointer_settings();

    EXPECT_THAT(optional_settings.is_set(), Eq(true));

    ptr_settings = optional_settings.value();
    EXPECT_THAT(ptr_settings.handedness, Eq(mir_pointer_handedness_left));
    EXPECT_THAT(ptr_settings.cursor_acceleration_bias, Eq(0.0));
    EXPECT_THAT(ptr_settings.horizontal_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.vertical_scroll_scale, Eq(1.0));
    EXPECT_THAT(ptr_settings.acceleration_profile, Eq(mir_pointer_acceleration_profile_adaptive));
}

TEST_F(LibInputDeviceOnMouse, applies_pointer_settings)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_profile_none);
    mi::PointerSettings settings(mouse.get_pointer_settings().value());
    settings.cursor_acceleration_bias = 1.1;
    settings.handedness = mir_pointer_handedness_left;
    settings.acceleration_profile = mir_pointer_acceleration_profile_flat;

    EXPECT_CALL(mock_libinput, libinput_device_config_accel_set_speed(mouse.device(), 1.1)).Times(1);
    EXPECT_CALL(mock_libinput, libinput_device_config_accel_set_profile(mouse.device(), LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)).Times(1);
    EXPECT_CALL(mock_libinput, libinput_device_config_left_handed_set(mouse.device(), true)).Times(1);

    mouse.apply_settings(settings);
}

TEST_F(LibInputDeviceOnLaptopKeyboardAndMouse, denies_pointer_settings_on_keyboards)
{
    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_profile_none);
    auto settings_from_mouse = mouse.get_pointer_settings();

    EXPECT_CALL(mock_libinput,libinput_device_config_accel_set_speed(_, _)).Times(0);
    EXPECT_CALL(mock_libinput,libinput_device_config_left_handed_set(_, _)).Times(0);

    keyboard.apply_settings(settings_from_mouse.value());
}

TEST_F(LibInputDeviceOnMouse, scroll_speed_scales_scroll_events)
{
    setup_axis_event(fake_event_1, event_time_1, 0.0, 3.0);
    setup_axis_event(fake_event_2, event_time_2, -2.0, 0.0);

    // expect two scroll events..
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 3.0f)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, -10.0f)));

    setup_pointer_configuration(mouse.device(), 1, mir_pointer_handedness_right, mir_pointer_acceleration_profile_flat);
    mi::PointerSettings settings(mouse.get_pointer_settings().value());
    settings.vertical_scroll_scale = -1.0;
    settings.horizontal_scroll_scale = 5.0;
    mouse.apply_settings(settings);

    mouse.start(&mock_sink, &mock_builder);
    mouse.process_event(fake_event_1);
    mouse.process_event(fake_event_2);
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
    setup_finger_axis_event(fake_event_1, event_time_1, 0.0, 150.0);
    setup_finger_axis_event(fake_event_2, event_time_2, 15.0, 0.0);

    InSequence seq;
    // expect two scroll events..
    EXPECT_CALL(mock_builder,
                pointer_event(time_stamp_1, mir_pointer_action_motion, 0, 0.0f, -10.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, -10.0f)));
    EXPECT_CALL(mock_builder,
                pointer_event(time_stamp_2, mir_pointer_action_motion, 0, 1.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 1.0f)));

    touchpad.start(&mock_sink, &mock_builder);
    touchpad.process_event(fake_event_1);
    touchpad.process_event(fake_event_2);

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
    EXPECT_THAT(settings.disable_with_mouse, Eq(true));
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
    settings.disable_with_mouse = true;
    settings.middle_mouse_button_emulation = true;

    EXPECT_CALL(mock_libinput,
                libinput_device_config_scroll_set_method(touchpad.device(), LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN));
    EXPECT_CALL(mock_libinput,
                libinput_device_config_click_set_method(touchpad.device(), LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER));
    EXPECT_CALL(mock_libinput, libinput_device_config_scroll_set_button(touchpad.device(), KEY_A));
    EXPECT_CALL(mock_libinput, libinput_device_config_tap_set_enabled(touchpad.device(), LIBINPUT_CONFIG_TAP_ENABLED));
    EXPECT_CALL(mock_libinput,
                libinput_device_config_dwt_set_enabled(touchpad.device(), LIBINPUT_CONFIG_DWT_DISABLED));
    EXPECT_CALL(mock_libinput, libinput_device_config_send_events_set_mode(
                                   touchpad.device(), LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE));
    EXPECT_CALL(mock_libinput, libinput_device_config_middle_emulation_set_enabled(
                                   touchpad.device(), LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED));

    touchpad.apply_settings(settings);
}

TEST_F(LibInputDevice, device_ptr_keeps_libinput_context_alive)
{
    InSequence seq;
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device));
    EXPECT_CALL(mock_libinput, libinput_device_unref(fake_device));
    EXPECT_CALL(mock_libinput, libinput_unref(fake_input));

    mock_libinput.setup_device(fake_input, fake_device, "/dev/test/path", "name", 1, 2);

    auto device_ptr = mie::make_libinput_device(lib, "/dev/test/path");

    lib.reset();
    device_ptr.reset();
}
