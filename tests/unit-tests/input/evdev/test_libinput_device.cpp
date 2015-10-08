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
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_libinput.h"
#include "mir/test/gmock_fixes.h"
#include "mir_test_framework/udev_environment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>

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
    mi::DefaultEventBuilder builder{MirInputDeviceId{3}};
    MockEventBuilder()
    {
        ON_CALL(*this, key_event(_,_,_,_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirKeyboardAction action, xkb_keysym_t key, int scan_code,
                                         MirInputEventModifiers modifier)
                                  {
                                        return builder.key_event(time, action, key, scan_code, modifier);
                                  }));
        ON_CALL(*this, touch_event(_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirInputEventModifiers modifier)
                                  {
                                        return builder.touch_event(time, modifier);
                                  }));
        ON_CALL(*this, add_touch(_,_,_,_,_,_,_,_,_,_))
            .WillByDefault(Invoke([this](MirEvent& event, MirTouchId id, MirTouchAction action,
                                         MirTouchTooltype tooltype, float x, float y, float major, float minor,
                                         float pressure, float size)
                                  {
                                        return builder.add_touch(event, id, action, tooltype, x, y, major, minor,
                                                                 pressure, size);
                                  }));
        ON_CALL(*this, pointer_event(_,_,_,_,_,_,_,_,_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirInputEventModifiers modifier, MirPointerAction action,
                                         MirPointerButtons buttons, float x, float y, float hscroll, float vscroll,
                                         float relative_x, float relative_y)
                                  {
                                      return builder.pointer_event(time, modifier, action, buttons, x, y, hscroll,
                                                                   vscroll, relative_x, relative_y);
                                  }));
        ON_CALL(*this, configuration_event(_,_))
            .WillByDefault(Invoke([this](Timestamp time, MirInputConfigurationAction action)
                                  {
                                        return builder.configuration_event(time, action);
                                  }));
    }
    using EventBuilder::Timestamp;
    MOCK_METHOD5(key_event, mir::EventUPtr(Timestamp, MirKeyboardAction, xkb_keysym_t, int, MirInputEventModifiers));

    MOCK_METHOD2(touch_event, mir::EventUPtr(Timestamp, MirInputEventModifiers));
    MOCK_METHOD10(add_touch, void(MirEvent&, MirTouchId, MirTouchAction, MirTouchTooltype, float, float, float, float,
                                  float, float));

    MOCK_METHOD10(pointer_event, mir::EventUPtr(Timestamp, MirInputEventModifiers, MirPointerAction, MirPointerButtons,
                                                float, float, float, float, float, float));
    MOCK_METHOD2(configuration_event, mir::EventUPtr(Timestamp, MirInputConfigurationAction));
};

struct LibInputDevice : public ::testing::Test
{
    mtf::UdevEnvironment env;
    ::testing::NiceMock<mir::test::doubles::MockLibInput> mock_libinput;
    ::testing::NiceMock<MockInputSink> mock_sink;
    ::testing::NiceMock<MockEventBuilder> mock_builder;

    libinput* fake_input = reinterpret_cast<libinput*>(0xF4C3);
    libinput_device* fake_device = reinterpret_cast<libinput_device*>(0xF4C4);
    libinput_event* fake_event_1 = reinterpret_cast<libinput_event*>(0xF4C5);
    libinput_event* fake_event_2 = reinterpret_cast<libinput_event*>(0xF4C6);
    libinput_event* fake_event_3 = reinterpret_cast<libinput_event*>(0xF4C7);
    libinput_event* fake_event_4 = reinterpret_cast<libinput_event*>(0xF4C8);
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
    char const* path{laptop_keyboard_device_path};

    LibInputDevice()
    {
        setup_device(fake_device, path, "laptop-keyboard", 5252, 3113);

        ON_CALL(mock_libinput, libinput_path_create_context(_,_))
            .WillByDefault(Return(fake_input));
    }

    void setup_device(libinput_device* dev, char const* device_path, char const* umock_name, unsigned int vendor_id, unsigned int product_id)
    {
        env.add_standard_device(umock_name);
        mock_libinput.setup_device(fake_input, dev, device_path, umock_name, vendor_id, product_id);
    }

    void setup_pointer_configuration(libinput_device* dev, double accel_speed, MirPointerButton primary)
    {
        EXPECT_CALL(mock_libinput, libinput_device_config_accel_get_speed(dev))
            .WillRepeatedly(Return(accel_speed));
        EXPECT_CALL(mock_libinput, libinput_device_config_left_handed_get(dev))
            .WillRepeatedly(Return(primary == mir_pointer_button_secondary));
    }

    void setup_key_event(libinput_event* event, uint64_t event_time, uint32_t key, libinput_key_state state)
    {
        auto key_event = reinterpret_cast<libinput_event_keyboard*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_KEYBOARD_KEY));
        EXPECT_CALL(mock_libinput, libinput_event_get_keyboard_event(event))
            .WillRepeatedly(Return(key_event));
        EXPECT_CALL(mock_libinput, libinput_event_keyboard_get_time_usec(key_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_keyboard_get_key(key_event))
            .WillRepeatedly(Return(key));
        EXPECT_CALL(mock_libinput, libinput_event_keyboard_get_key_state(key_event))
            .WillRepeatedly(Return(state));
    }

    void setup_pointer_event(libinput_event* event, uint64_t event_time, float relatve_x, float relatve_y)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_MOTION));
        EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillRepeatedly(Return(pointer_event));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dx(pointer_event))
            .WillRepeatedly(Return(relatve_x));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dy(pointer_event))
            .WillRepeatedly(Return(relatve_y));
    }

    void setup_button_event(libinput_event* event, uint64_t event_time, int button, libinput_button_state state)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_BUTTON));
        EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillRepeatedly(Return(pointer_event));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_button(pointer_event))
            .WillRepeatedly(Return(button));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_button_state(pointer_event))
            .WillRepeatedly(Return(state));
    }

    void setup_axis_event(libinput_event* event, uint64_t event_time, double horizontal, double vertical)
    {
        auto pointer_event = reinterpret_cast<libinput_event_pointer*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_AXIS));
        EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(event))
            .WillRepeatedly(Return(pointer_event));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time_usec(pointer_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillRepeatedly(Return(horizontal!=0.0));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillRepeatedly(Return(vertical!=0.0));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
            .WillRepeatedly(Return(vertical));
        EXPECT_CALL(mock_libinput, libinput_event_pointer_get_axis_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
            .WillRepeatedly(Return(horizontal));
    }

    void setup_touch_event(libinput_event* event, libinput_event_type type, uint64_t event_time, int slot, float x,
                           float y, float major, float minor, float pressure)
    {
        auto touch_event = reinterpret_cast<libinput_event_touch*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(type));
        EXPECT_CALL(mock_libinput, libinput_event_get_touch_event(event))
            .WillRepeatedly(Return(touch_event));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_slot(touch_event))
            .WillRepeatedly(Return(slot));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_x_transformed(touch_event, _))
            .WillRepeatedly(Return(x));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_y_transformed(touch_event, _))
            .WillRepeatedly(Return(y));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_time_usec(touch_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_major_transformed(touch_event, _, _))
            .WillRepeatedly(Return(major));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_minor_transformed(touch_event, _, _))
            .WillRepeatedly(Return(minor));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_pressure(touch_event))
            .WillRepeatedly(Return(pressure));
    }

    void setup_touch_up_event(libinput_event* event, uint64_t event_time, int slot)
    {
        auto touch_event = reinterpret_cast<libinput_event_touch*>(event);

        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_TOUCH_UP));
        EXPECT_CALL(mock_libinput, libinput_event_get_touch_event(event))
            .WillRepeatedly(Return(touch_event));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_slot(touch_event))
            .WillRepeatedly(Return(slot));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_x_transformed(touch_event, _))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_y_transformed(touch_event, _))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_x(touch_event))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_y(touch_event))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_time_usec(touch_event))
            .WillRepeatedly(Return(event_time));
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_major_transformed(touch_event, _, _))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_minor_transformed(touch_event, _, _))
            .Times(0);
        EXPECT_CALL(mock_libinput, libinput_event_touch_get_pressure(touch_event))
            .Times(0);

    }

    void setup_touch_frame(libinput_event* event)
    {
        EXPECT_CALL(mock_libinput, libinput_event_get_type(event))
            .WillRepeatedly(Return(LIBINPUT_EVENT_TOUCH_FRAME));
    }
};

}

TEST_F(LibInputDevice, start_creates_and_unrefs_libinput_device_from_path)
{
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(path)))
        .Times(1);
    // according to manual libinput_path_add_device creates a temporary device with a ref count 0.
    // hence it needs a manual ref call
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device))
        .Times(1);
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(),
                            path,
                            std::move(mie::make_libinput_device(lib.get(), path)));
    dev.start(&mock_sink, &mock_builder);
}

TEST_F(LibInputDevice, open_device_of_group)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    char const* second_dev = "/dev/input/event13";
    char const* second_umock_dev_name = "bluetooth-magic-trackpad";
    
    setup_device(second_fake_device, second_dev, second_umock_dev_name, 9663, 1234);

    InSequence seq;
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(path))).Times(1);
    // according to manual libinput_path_add_device creates a temporary device with a ref count 0.
    // hence it needs a manual ref call
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device)).Times(1);
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(second_dev))).Times(1);
    EXPECT_CALL(mock_libinput, libinput_device_ref(second_fake_device)).Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            path,
                            std::move(mie::make_libinput_device(lib.get(), path)));
    dev.add_device_of_group(second_dev, mie::make_libinput_device(lib.get(), second_dev));
    dev.start(&mock_sink, &mock_builder);
}

TEST_F(LibInputDevice, input_info_combines_capabilities)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    char const* second_dev = "/dev/input/event13";
    char const* second_umock_dev_name = "bluetooth-magic-trackpad";
    
    setup_device(second_fake_device, second_dev, second_umock_dev_name, 9663, 1234);

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            path,
                            mie::make_libinput_device(lib.get(), path));
    dev.add_device_of_group(second_dev, mie::make_libinput_device(lib.get(), second_dev));
    auto info = dev.get_device_info();

    EXPECT_THAT(info.capabilities, Eq(mi::DeviceCapability::touchpad|
                                      mi::DeviceCapability::keyboard|
                                      mi::DeviceCapability::alpha_numeric));
}

TEST_F(LibInputDevice, removal_unrefs_libinput_device)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();

    EXPECT_CALL(mock_libinput, libinput_device_unref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));
}


TEST_F(LibInputDevice, process_event_converts_key_event)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    setup_key_event(fake_event_1, event_time_1, KEY_A, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_2, event_time_2, KEY_A, LIBINPUT_KEY_STATE_RELEASED);

    EXPECT_CALL(mock_builder, key_event(time_stamp_1, mir_keyboard_action_down, _, KEY_A, mir_input_event_modifier_none));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_2, mir_keyboard_action_up, _, KEY_A, mir_input_event_modifier_none));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_A),mt::KeyUpEvent())));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
}

TEST_F(LibInputDevice, process_event_accumulates_key_state)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));


    setup_key_event(fake_event_1, event_time_1, KEY_C, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_2, event_time_2, KEY_LEFTALT, LIBINPUT_KEY_STATE_PRESSED);
    setup_key_event(fake_event_3, event_time_3, KEY_C, LIBINPUT_KEY_STATE_RELEASED);

    InSequence seq;
    EXPECT_CALL(mock_builder, key_event(time_stamp_1, mir_keyboard_action_down, _, KEY_C, mir_input_event_modifier_none));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_2, mir_keyboard_action_down, _, KEY_LEFTALT, mir_input_event_modifier_none));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_LEFTALT),mt::KeyDownEvent())));
    EXPECT_CALL(mock_builder, key_event(time_stamp_3, mir_keyboard_action_up, _, KEY_C, mir_input_event_modifier_alt|mir_input_event_modifier_alt_left));
    EXPECT_CALL(mock_sink, handle_input(AllOf(mt::KeyOfScanCode(KEY_C),
                                              mt::KeyWithModifiers(
                                                  MirInputEventModifiers{
                                                      mir_input_event_modifier_alt|
                                                      mir_input_event_modifier_alt_left
                                                  }),
                                              mt::KeyUpEvent())));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
    dev.process_event(fake_event_3);
}

TEST_F(LibInputDevice, process_event_converts_pointer_event)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    float x = 15;
    float y = 17;
    setup_pointer_event(fake_event_1, event_time_1, x, y);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x,y)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
}

TEST_F(LibInputDevice, process_event_provides_relative_coordinates)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    float x = -5;
    float y = 20;
    setup_pointer_event(fake_event_1, event_time_1, x, y);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x,y)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
}

TEST_F(LibInputDevice, process_event_accumulates_pointer_movement)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    float x1 = 15, x2 = 23;
    float y1 = 17, y2 = 21;

    setup_pointer_event(fake_event_1, event_time_1, x1, y1);
    setup_pointer_event(fake_event_2, event_time_2, x2, y2);

    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x1,y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x1+x2,y1+y2)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
}

TEST_F(LibInputDevice, process_event_handles_press_and_release)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));
    float const x = 0;
    float const y = 0;
    geom::Point const pos{x,y};

    setup_button_event(fake_event_1, event_time_1, BTN_LEFT, LIBINPUT_BUTTON_STATE_PRESSED);
    setup_button_event(fake_event_2, event_time_2, BTN_RIGHT, LIBINPUT_BUTTON_STATE_PRESSED);
    setup_button_event(fake_event_3, event_time_3, BTN_RIGHT, LIBINPUT_BUTTON_STATE_RELEASED);
    setup_button_event(fake_event_4, event_time_4, BTN_LEFT, LIBINPUT_BUTTON_STATE_RELEASED);

    InSequence seq;
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_primary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_secondary)));
    EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEventWithButton(pos, mir_pointer_button_primary)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
    dev.process_event(fake_event_3);
    dev.process_event(fake_event_4);
}

TEST_F(LibInputDevice, process_event_handles_scroll)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    setup_axis_event(fake_event_1, event_time_1, 0.0, 20.0);
    setup_axis_event(fake_event_2, event_time_2, 5.0, 0.0);

    InSequence seq;
    // expect two scroll events..
    EXPECT_CALL(mock_builder, pointer_event(time_stamp_1, mir_input_event_modifier_none, mir_pointer_action_motion, 0, 0.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 20.0f)));
    EXPECT_CALL(mock_builder, pointer_event(time_stamp_2, mir_input_event_modifier_none, mir_pointer_action_motion, 0, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 5.0f)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
}

TEST_F(LibInputDevice, process_event_handles_touch_down_events)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    int slot = 0;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 100;
    float y = 7;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1, mir_input_event_modifier_none));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{0}, mir_touch_action_down, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x,y)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
}

TEST_F(LibInputDevice, process_event_handles_touch_move_events)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    int slot = 0;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 100;
    float y = 7;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_MOTION, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1, mir_input_event_modifier_none));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{0}, mir_touch_action_change, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchMovementEvent()));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
}

TEST_F(LibInputDevice, process_event_handles_touch_up_events_without_querying_properties)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    int slot = 3;
    float major = 6;
    float minor = 5;
    float pressure = 0.6f;
    float x = 30;
    float y = 20;

    setup_touch_event(fake_event_1, LIBINPUT_EVENT_TOUCH_DOWN, event_time_1, slot, x, y, major, minor, pressure);
    setup_touch_frame(fake_event_2);
    setup_touch_up_event(fake_event_3, event_time_2, slot);
    setup_touch_frame(fake_event_4);

    InSequence seq;
    EXPECT_CALL(mock_builder, touch_event(time_stamp_1, mir_input_event_modifier_none));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{slot}, mir_touch_action_down, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchEvent(x,y)));

    EXPECT_CALL(mock_builder, touch_event(time_stamp_2, mir_input_event_modifier_none));
    EXPECT_CALL(mock_builder, add_touch(_, MirTouchId{slot}, mir_touch_action_up, mir_touch_tooltype_finger, x, y,
                                        pressure, major, minor, major));
    EXPECT_CALL(mock_sink, handle_input(mt::TouchUpEvent(x,y)));

    dev.start(&mock_sink, &mock_builder);
    dev.process_event(fake_event_1);
    dev.process_event(fake_event_2);
    dev.process_event(fake_event_3);
    dev.process_event(fake_event_4);
}
