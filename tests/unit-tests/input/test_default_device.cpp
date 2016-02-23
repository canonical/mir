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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "src/server/input/default_device.h"
#include "mir/input/input_device.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/pointer_configuration.h"
#include "mir/dispatch/action_queue.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mi = mir::input;
namespace md = mir::dispatch;
using namespace ::testing;
namespace
{
struct MockInputDevice : public mi::InputDevice
{
    MOCK_METHOD2(start, void(mi::InputSink* destination, mi::EventBuilder* builder));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(get_device_info, mi::InputDeviceInfo());
    MOCK_CONST_METHOD0(get_pointer_settings, mir::optional_value<mi::PointerSettings>());
    MOCK_METHOD1(apply_settings, void(mi::PointerSettings const&));
    MOCK_CONST_METHOD0(get_touchpad_settings, mir::optional_value<mi::TouchpadSettings>());
    MOCK_METHOD1(apply_settings, void(mi::TouchpadSettings const&));
};

struct DefaultDevice : Test
{
    NiceMock<MockInputDevice> touchpad;
    NiceMock<MockInputDevice> mouse;
    NiceMock<MockInputDevice> keyboard;
    std::shared_ptr<md::ActionQueue> queue{std::make_shared<md::ActionQueue>()};

    DefaultDevice()
    {
        using optional_pointer_settings = mir::optional_value<mi::PointerSettings>;
        using optional_touchpad_settings = mir::optional_value<mi::TouchpadSettings>;
        ON_CALL(touchpad, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer}));
        ON_CALL(touchpad, get_pointer_settings())
            .WillByDefault(Return(optional_pointer_settings{mi::PointerSettings{}}));
        ON_CALL(touchpad, get_touchpad_settings())
            .WillByDefault(Return(optional_touchpad_settings{mi::TouchpadSettings{}}));

        ON_CALL(mouse, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::pointer}));
        ON_CALL(mouse, get_pointer_settings()).WillByDefault(Return(optional_pointer_settings{mi::PointerSettings{}}));
        ON_CALL(mouse, get_touchpad_settings()).WillByDefault(Return(optional_touchpad_settings{}));

        ON_CALL(keyboard, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::keyboard}));
        ON_CALL(keyboard, get_pointer_settings()).WillByDefault(Return(optional_pointer_settings{}));
        ON_CALL(keyboard, get_touchpad_settings()).WillByDefault(Return(optional_touchpad_settings{}));
    }
};

}

TEST_F(DefaultDevice, refuses_touchpad_config_on_mice)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, mouse);
    mi::TouchpadConfiguration touch_conf;

    EXPECT_THROW({dev.apply_touchpad_configuration(touch_conf);}, std::invalid_argument);
}

TEST_F(DefaultDevice, refuses_touchpad_and_pointer_settings_on_keyboards)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, keyboard);
    mi::TouchpadConfiguration touch_conf;
    mi::PointerConfiguration pointer_conf;

    EXPECT_THROW({dev.apply_touchpad_configuration(touch_conf);}, std::invalid_argument);
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);
}


TEST_F(DefaultDevice, accepts_pointer_config_on_mice)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, mouse);
    mi::PointerConfiguration pointer_conf;

    EXPECT_CALL(mouse, apply_settings(Matcher<mi::PointerSettings const&>(_)));

    dev.apply_pointer_configuration(pointer_conf);
    queue->dispatch(md::FdEvent::readable);
}

TEST_F(DefaultDevice, accepts_touchpad_and_pointer_config_on_touchpads)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad);
    mi::TouchpadConfiguration touch_conf;
    mi::PointerConfiguration pointer_conf;

    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::PointerSettings const&>(_)));
    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::TouchpadSettings const&>(_)));

    dev.apply_touchpad_configuration(touch_conf);
    dev.apply_pointer_configuration(pointer_conf);
    queue->dispatch(md::FdEvent::readable);
    queue->dispatch(md::FdEvent::readable);
}

TEST_F(DefaultDevice, ensures_cursor_accleration_bias_is_in_range)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad);

    mi::PointerConfiguration pointer_conf;
    pointer_conf.cursor_acceleration_bias = 3.0;
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);
    pointer_conf.cursor_acceleration_bias = -2.0;
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);

    pointer_conf.cursor_acceleration_bias = 1.0;
    EXPECT_NO_THROW(dev.apply_pointer_configuration(pointer_conf));
}
