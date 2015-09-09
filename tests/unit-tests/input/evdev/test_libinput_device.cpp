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
#include "mir/event_printer.h"
#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_libinput.h"
#include "mir/test/gmock_fixes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>

namespace mi = mir::input;
namespace mie = mi::evdev;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

class StubInputDeviceRegistry : public mi::InputDeviceRegistry
{
public:
    void add_device(std::shared_ptr<mi::InputDevice> const&) override {}
    void remove_device(std::shared_ptr<mi::InputDevice> const&) override {}
};

struct MockInputSink : mi::InputSink
{
    MOCK_METHOD1(handle_input,void(MirEvent &));
    MOCK_METHOD1(confine_pointer, void(mir::geometry::Point&));
    MOCK_CONST_METHOD0(bounding_rectangle, mir::geometry::Rectangle());
};

struct MockEventBuilder : mi::EventBuilder
{
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
    ::testing::NiceMock<mir::test::doubles::MockLibInput> mock_libinput;
    ::testing::NiceMock<MockInputSink> mock_sink;
    mi::DefaultEventBuilder builder{MirInputDeviceId{3}};

    libinput* fake_input = reinterpret_cast<libinput*>(0xF4C3);
    libinput_device* fake_device = reinterpret_cast<libinput_device*>(0xF4C4);
    libinput_event* fake_event = reinterpret_cast<libinput_event*>(0xF4C5);
    libinput_event_keyboard* fake_keyboard_event = reinterpret_cast<libinput_event_keyboard*>(0xF46C);
    libinput_event_pointer* fake_pointer_event = reinterpret_cast<libinput_event_pointer*>(0xF4C6);
    libinput_event_touch* fake_touch_event = reinterpret_cast<libinput_event_touch*>(0xF4C7);
    libinput_device* second_fake_device = reinterpret_cast<libinput_device*>(0xF4C9);

    char const* path = "/path/to/dev";

    LibInputDevice()
    {
        using namespace ::testing;
        ON_CALL(mock_libinput, libinput_path_create_context(_,_))
            .WillByDefault(Return(fake_input));
        ON_CALL(mock_libinput, libinput_path_add_device(fake_input,_))
            .WillByDefault(Return(fake_device));
        ON_CALL(mock_libinput, libinput_device_ref(fake_device))
            .WillByDefault(Return(fake_device));
        ON_CALL(mock_libinput, libinput_device_unref(fake_device))
            .WillByDefault(Return(nullptr));
    }

};

}
using namespace ::testing;

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
                            mie::make_libinput_device(lib.get(), path));
    dev.start(&mock_sink, &builder);
}

TEST_F(LibInputDevice, open_device_of_group)
{
    char const* first_dev = "/path/to/dev1";
    char const* second_dev = "/path/to/dev2";
    std::shared_ptr<libinput> lib = mie::make_libinput();

    InSequence seq;
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(first_dev)));
    // according to manual libinput_path_add_device creates a temporary device with a ref count 0.
    // hence it needs a manual ref call
    EXPECT_CALL(mock_libinput, libinput_device_ref(fake_device));
    EXPECT_CALL(mock_libinput, libinput_path_add_device(fake_input,StrEq(second_dev)))
        .WillOnce(Return(second_fake_device));
    EXPECT_CALL(mock_libinput, libinput_device_ref(second_fake_device));

    mie::LibInputDevice dev(mir::report::null_input_report(),
                            first_dev,
                            mie::make_libinput_device(lib.get(), first_dev));
    dev.add_device_of_group(second_dev, mie::make_libinput_device(lib.get(), second_dev));
    dev.start(&mock_sink, &builder);
}

TEST_F(LibInputDevice, removal_unrefs_libinput_device)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();

    EXPECT_CALL(mock_libinput, libinput_device_unref(fake_device))
        .Times(1);

    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));
}

TEST_F(LibInputDevice, process_event_converts_pointer_event)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    uint32_t event_time = 14;
    float x = 15;
    float y = 17;

    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillOnce(Return(LIBINPUT_EVENT_POINTER_MOTION));
    EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(fake_event))
        .WillOnce(Return(fake_pointer_event));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time(fake_pointer_event))
        .WillOnce(Return(event_time));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dx(fake_pointer_event))
        .WillOnce(Return(x));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dy(fake_pointer_event))
        .WillOnce(Return(y));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x,y)));

    dev.start(&mock_sink, &builder);
    dev.process_event(fake_event);
}

TEST_F(LibInputDevice, process_event_provides_relative_coordinates)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    uint32_t event_time = 14;
    float x = -5;
    float y = 20;

    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillOnce(Return(LIBINPUT_EVENT_POINTER_MOTION));
    EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(fake_event))
        .WillOnce(Return(fake_pointer_event));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time(fake_pointer_event))
        .WillOnce(Return(event_time));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dx(fake_pointer_event))
        .WillOnce(Return(x));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dy(fake_pointer_event))
        .WillOnce(Return(y));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithDiff(x,y)));

    dev.start(&mock_sink, &builder);
    dev.process_event(fake_event);
}

TEST_F(LibInputDevice, process_event_accumulates_pointer_movement)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    uint32_t event_time = 14;
    float x1 = 15, x2 = 23;
    float y1 = 17, y2 = 21;

    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_MOTION));
    EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(fake_event))
        .WillRepeatedly(Return(fake_pointer_event));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time(fake_pointer_event))
        .WillRepeatedly(Return(event_time));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dx(fake_pointer_event))
        .WillOnce(Return(x1))
        .WillOnce(Return(x2));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_dy(fake_pointer_event))
        .WillOnce(Return(y1))
        .WillOnce(Return(y2));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x1,y1)));
    EXPECT_CALL(mock_sink, handle_input(mt::PointerEventWithPosition(x1+x2,y1+y2)));

    dev.start(&mock_sink, &builder);
    dev.process_event(fake_event);
    dev.process_event(fake_event);
}

TEST_F(LibInputDevice, process_event_handles_press_and_release)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    uint32_t event_time = 14;

    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_BUTTON));
    EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(fake_event))
        .WillRepeatedly(Return(fake_pointer_event));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time(fake_pointer_event))
        .WillRepeatedly(Return(event_time));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_button(fake_pointer_event))
        .WillOnce(Return(BTN_LEFT))
        .WillOnce(Return(BTN_RIGHT))
        .WillOnce(Return(BTN_RIGHT))
        .WillOnce(Return(BTN_LEFT));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_button_state(fake_pointer_event))
        .WillOnce(Return(LIBINPUT_BUTTON_STATE_PRESSED))
        .WillOnce(Return(LIBINPUT_BUTTON_STATE_PRESSED))
        .WillOnce(Return(LIBINPUT_BUTTON_STATE_RELEASED))
        .WillOnce(Return(LIBINPUT_BUTTON_STATE_RELEASED));
    {
        InSequence seq;
        EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEventWithButton(0,0,1)));
        EXPECT_CALL(mock_sink, handle_input(mt::ButtonDownEvent(0,0)));
        EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEvent(0,0)));
        EXPECT_CALL(mock_sink, handle_input(mt::ButtonUpEvent(0,0)));
    }

    dev.start(&mock_sink, &builder);
    dev.process_event(fake_event);
    dev.process_event(fake_event);
    dev.process_event(fake_event);
    dev.process_event(fake_event);
}

TEST_F(LibInputDevice, process_event_handles_scoll)
{
    std::shared_ptr<libinput> lib = mie::make_libinput();
    mie::LibInputDevice dev(mir::report::null_input_report(), path, mie::make_libinput_device(lib.get(), path));

    uint32_t event_time = 14;

    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillRepeatedly(Return(LIBINPUT_EVENT_POINTER_AXIS));
    EXPECT_CALL(mock_libinput, libinput_event_get_pointer_event(fake_event))
        .WillRepeatedly(Return(fake_pointer_event));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_time(fake_pointer_event))
        .WillRepeatedly(Return(event_time));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_has_axis(fake_pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillOnce(Return(0))
        .WillOnce(Return(1));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_has_axis(fake_pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_axis_value(fake_pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
        .WillOnce(Return(20.0f));
    EXPECT_CALL(mock_libinput, libinput_event_pointer_get_axis_value(fake_pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
        .WillOnce(Return(5.0f));
    {
        InSequence seq;
        // expect two scroll events..
        EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_vscroll, 20.0f)));
        EXPECT_CALL(mock_sink, handle_input(mt::PointerAxisChange(mir_pointer_axis_hscroll, 5.0f)));
    }

    dev.start(&mock_sink, &builder);
    dev.process_event(fake_event);
    dev.process_event(fake_event);
}

