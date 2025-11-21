/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/server/input/default_device.h"
#include "mir/input/input_device.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/test/doubles/mock_key_mapper.h"
#include "mir/test/doubles/stub_keymap.h"
#include "mir/dispatch/action_queue.h"
#include "mir/test/fake_shared.h"
#include "mir/events/event_private.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using namespace ::testing;
namespace
{
struct MockInputDevice : public mi::InputDevice
{
    MOCK_METHOD(void, start, (mi::InputSink* destination, mi::EventBuilder* builder), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(mi::InputDeviceInfo, get_device_info, (), (override));
    MOCK_METHOD(mir::optional_value<mi::PointerSettings>, get_pointer_settings, (), (const, override));
    MOCK_METHOD(void, apply_settings, (mi::PointerSettings const&), (override));
    MOCK_METHOD(mir::optional_value<mi::TouchpadSettings>, get_touchpad_settings, (), (const, override));
    MOCK_METHOD(void, apply_settings, (mi::TouchpadSettings const&), (override));
    MOCK_METHOD(mir::optional_value<mi::TouchscreenSettings>, get_touchscreen_settings, (), (const, override));
    MOCK_METHOD(void, apply_settings, (mi::TouchscreenSettings const&), (override));
};

struct DefaultDevice : Test
{
    NiceMock<MockInputDevice> touchpad;
    NiceMock<MockInputDevice> mouse;
    NiceMock<MockInputDevice> keyboard;
    NiceMock<MockInputDevice> touchscreen;
    NiceMock<mtd::MockKeyMapper> key_mapper;
    std::shared_ptr<md::ActionQueue> queue{std::make_shared<md::ActionQueue>()};
    std::function<void(mi::Device*)> const change_callback{[](mi::Device*){}};

    DefaultDevice()
    {
        using optional_pointer_settings = mir::optional_value<mi::PointerSettings>;
        using optional_touchpad_settings = mir::optional_value<mi::TouchpadSettings>;
        using optional_touchscreen_settings = mir::optional_value<mi::TouchscreenSettings>;
        ON_CALL(touchpad, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer}));
        ON_CALL(touchpad, get_pointer_settings())
            .WillByDefault(Return(optional_pointer_settings{mi::PointerSettings{}}));
        ON_CALL(touchpad, get_touchpad_settings())
            .WillByDefault(Return(optional_touchpad_settings{mi::TouchpadSettings{}}));
        ON_CALL(touchpad, get_touchscreen_settings()).WillByDefault(Return(optional_touchscreen_settings{}));

        ON_CALL(mouse, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::pointer}));
        ON_CALL(mouse, get_pointer_settings()).WillByDefault(Return(optional_pointer_settings{mi::PointerSettings{}}));
        ON_CALL(mouse, get_touchpad_settings()).WillByDefault(Return(optional_touchpad_settings{}));
        ON_CALL(mouse, get_touchscreen_settings()).WillByDefault(Return(optional_touchscreen_settings{}));

        ON_CALL(keyboard, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{
                "name", "unique", mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric}));
        ON_CALL(keyboard, get_pointer_settings()).WillByDefault(Return(optional_pointer_settings{}));
        ON_CALL(keyboard, get_touchpad_settings()).WillByDefault(Return(optional_touchpad_settings{}));
        ON_CALL(keyboard, get_touchscreen_settings()).WillByDefault(Return(optional_touchscreen_settings{}));

        ON_CALL(touchscreen, get_device_info())
            .WillByDefault(Return(mi::InputDeviceInfo{"name", "unique", mi::DeviceCapability::touchscreen}));
        ON_CALL(touchscreen, get_pointer_settings()).WillByDefault(Return(optional_pointer_settings{}));
        ON_CALL(touchscreen, get_touchpad_settings()).WillByDefault(Return(optional_touchpad_settings{}));
        ON_CALL(touchscreen, get_touchscreen_settings())
            .WillByDefault(Return(optional_touchscreen_settings{mi::TouchscreenSettings{}}));
    }
};

}

TEST_F(DefaultDevice, refuses_touchpad_config_on_mice)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, mouse, mt::fake_shared(key_mapper));
    MirTouchpadConfig touch_conf;

    EXPECT_THROW({dev.apply_touchpad_configuration(touch_conf);}, std::invalid_argument);
}

TEST_F(DefaultDevice, refuses_touchpad_and_pointer_settings_on_keyboards)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, keyboard, mt::fake_shared(key_mapper));
    MirTouchpadConfig touch_conf;
    MirPointerConfig pointer_conf;

    EXPECT_THROW({dev.apply_touchpad_configuration(touch_conf);}, std::invalid_argument);
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);
}


TEST_F(DefaultDevice, accepts_pointer_config_on_mice)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, mouse, mt::fake_shared(key_mapper));
    MirPointerConfig pointer_conf;

    EXPECT_CALL(mouse, apply_settings(Matcher<mi::PointerSettings const&>(_)));

    dev.apply_pointer_configuration(pointer_conf);
    queue->dispatch(md::FdEvent::readable);
}

TEST_F(DefaultDevice, accepts_touchpad_and_pointer_config_on_touchpads)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad, mt::fake_shared(key_mapper));
    MirTouchpadConfig touch_conf;
    MirPointerConfig pointer_conf;

    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::PointerSettings const&>(_)));
    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::TouchpadSettings const&>(_)));

    dev.apply_touchpad_configuration(touch_conf);
    dev.apply_pointer_configuration(pointer_conf);
    queue->dispatch(md::FdEvent::readable);
    queue->dispatch(md::FdEvent::readable);
}

TEST_F(DefaultDevice, forwards_touchscreen_config_on_touchscreen)
{
    mi::DefaultDevice dev(MirInputDeviceId{23}, queue, touchscreen, mt::fake_shared(key_mapper));
    MirTouchscreenConfig touchscreen_conf;

    EXPECT_CALL(touchscreen, apply_settings(Matcher<mi::TouchscreenSettings const&>(_)));

    dev.apply_touchscreen_configuration(touchscreen_conf);
    queue->dispatch(md::FdEvent::readable);
}

TEST_F(DefaultDevice, ensures_cursor_accleration_bias_is_in_range)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad, mt::fake_shared(key_mapper));

    MirPointerConfig pointer_conf;
    pointer_conf.cursor_acceleration_bias(3.0);
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);
    pointer_conf.cursor_acceleration_bias(-2.0);
    EXPECT_THROW({dev.apply_pointer_configuration(pointer_conf);}, std::invalid_argument);

    pointer_conf.cursor_acceleration_bias(1.0);
    EXPECT_NO_THROW(dev.apply_pointer_configuration(pointer_conf));
}

TEST_F(DefaultDevice, touchpad_device_can_be_constructed_from_input_config)
{
    MirTouchpadConfig const tpd_conf{mir_touchpad_click_mode_finger_count, mir_touchpad_scroll_mode_edge_scroll, 0, false, true, true, true};
    MirPointerConfig const ptr_conf{mir_pointer_handedness_right, mir_pointer_acceleration_adaptive, 0.5, -1.0, 1.0};
    MirInputDevice conf(MirInputDeviceId{17}, mi::DeviceCapability::touchpad|mi::DeviceCapability::pointer, "touchpad", "toouchpad-event7");
    conf.set_touchpad_config(tpd_conf);
    conf.set_pointer_config(ptr_conf);
    mi::DefaultDevice dev(conf, queue, touchpad, mt::fake_shared(key_mapper), change_callback);

    EXPECT_EQ(tpd_conf, dev.touchpad_configuration().value());
    EXPECT_EQ(ptr_conf, dev.pointer_configuration().value());
}

TEST_F(DefaultDevice, pointer_device_can_be_constructed_from_input_config)
{
    MirPointerConfig const ptr_config{mir_pointer_handedness_left, mir_pointer_acceleration_none, 0, 1.0, 1.0};
    MirInputDevice conf(MirInputDeviceId{11}, mi::DeviceCapability::pointer, "pointer", "pointer-event7");
    conf.set_pointer_config(ptr_config);
    mi::DefaultDevice dev(conf, queue, mouse, mt::fake_shared(key_mapper), change_callback);

    EXPECT_EQ(ptr_config, dev.pointer_configuration().value());
}

TEST_F(DefaultDevice, keyboard_device_can_be_constructed_from_input_config)
{
    std::shared_ptr<mi::Keymap> keymap{std::make_shared<mtd::StubKeymap>()};
    MirKeyboardConfig const kbd_config{std::move(keymap)};
    MirInputDevice conf(MirInputDeviceId{3}, mi::DeviceCapability::keyboard, "keyboard", "keyboard-event3c");
    conf.set_keyboard_config(kbd_config);
    mi::DefaultDevice dev(conf, queue, keyboard, mt::fake_shared(key_mapper), change_callback);

    EXPECT_EQ(kbd_config, dev.keyboard_configuration().value());
}

TEST_F(DefaultDevice, touchscreen_device_can_be_constructed_from_input_config)
{
    MirTouchscreenConfig const ts_config{0, mir_touchscreen_mapping_mode_to_display_wall};
    MirInputDevice conf(MirInputDeviceId{5}, mi::DeviceCapability::touchscreen, "ts", "ts-event7");
    conf.set_touchscreen_config(ts_config);

    mi::DefaultDevice dev(conf, queue, touchscreen, mt::fake_shared(key_mapper), change_callback);

    EXPECT_EQ(ts_config, dev.touchscreen_configuration().value());
}

TEST_F(DefaultDevice, device_config_can_be_queried_from_touchpad)
{
    MirInputDeviceId const device_id{17};
    mi::DefaultDevice dev(device_id, queue, touchpad, mt::fake_shared(key_mapper), change_callback);

    auto dev_info = touchpad.get_device_info();
    MirInputDevice conf(device_id, dev_info.capabilities, dev_info.name, dev_info.unique_id);
    MirTouchpadConfig const tpd_conf{
        mir_touchpad_click_mode_finger_count,
        mir_touchpad_scroll_mode_edge_scroll,
        0,
        false,
        true,
        true,
        true};
    MirPointerConfig const ptr_conf{
        mir_pointer_handedness_right,
        mir_pointer_acceleration_adaptive,
        0.5,
        -1.0,
        1.0};

    conf.set_touchpad_config(tpd_conf);
    conf.set_pointer_config(ptr_conf);

    dev.apply_touchpad_configuration(tpd_conf);
    dev.apply_pointer_configuration(ptr_conf);

    EXPECT_EQ(conf, dev.config());
}

TEST_F(DefaultDevice, device_config_can_be_queried_from_pointer_device)
{
    MirInputDeviceId const device_id{11};
    mi::DefaultDevice dev(device_id, queue, mouse, mt::fake_shared(key_mapper), change_callback);

    auto dev_info = mouse.get_device_info();
    MirInputDevice conf(device_id, dev_info.capabilities, dev_info.name, dev_info.unique_id);
    MirPointerConfig const ptr_config{
        mir_pointer_handedness_left,
        mir_pointer_acceleration_none,
        0,
        1.0,
        1.0};

    conf.set_pointer_config(ptr_config);
    dev.apply_pointer_configuration(ptr_config);

    EXPECT_EQ(conf, dev.config());
}

TEST_F(DefaultDevice, device_config_can_be_queried_from_keyboard_device)
{
    MirInputDeviceId const device_id{3};
    mi::DefaultDevice dev(device_id, queue, keyboard, mt::fake_shared(key_mapper), change_callback);

    auto dev_info = keyboard.get_device_info();
    MirInputDevice conf(device_id, dev_info.capabilities, dev_info.name, dev_info.unique_id);
    std::shared_ptr<mi::Keymap> keymap{std::make_shared<mtd::StubKeymap>()};
    MirKeyboardConfig const kbd_config{std::move(keymap)};

    conf.set_keyboard_config(kbd_config);
    dev.apply_keyboard_configuration(kbd_config);

    EXPECT_EQ(kbd_config, dev.keyboard_configuration().value());
}

TEST_F(DefaultDevice, device_config_can_be_queried_from_touchscreen)
{
    MirInputDeviceId const device_id{5};
    mi::DefaultDevice dev(device_id, queue, touchscreen, mt::fake_shared(key_mapper), change_callback);

    auto dev_info = touchscreen.get_device_info();
    MirInputDevice conf(device_id, dev_info.capabilities, dev_info.name, dev_info.unique_id);
    MirTouchscreenConfig const ts_config{0, mir_touchscreen_mapping_mode_to_display_wall};

    conf.set_touchscreen_config(ts_config);
    dev.apply_touchscreen_configuration(ts_config);

    EXPECT_EQ(ts_config, dev.touchscreen_configuration().value());
}

TEST_F(DefaultDevice, disable_queue_ends_config_processing)
{
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad, mt::fake_shared(key_mapper));

    dev.disable_queue();
    MirPointerConfig pointer_conf;
    pointer_conf.cursor_acceleration_bias(1.0);

    EXPECT_CALL(touchpad, apply_settings(Matcher<mi::TouchpadSettings const&>(_))).Times(0);
    dev.apply_pointer_configuration(pointer_conf);
}

TEST_F(DefaultDevice, disable_queue_removes_reference_to_queue)
{
    std::weak_ptr<md::ActionQueue> ref = queue;
    mi::DefaultDevice dev(MirInputDeviceId{17}, queue, touchpad, mt::fake_shared(key_mapper));
    queue.reset();

    EXPECT_THAT(ref.lock(), Ne(nullptr));
    dev.disable_queue();
    EXPECT_THAT(ref.lock(), Eq(nullptr));
}

TEST_F(DefaultDevice, keyboard_device_can_be_constructed_from_input_config_with_broken_config)
{
    std::shared_ptr<mi::Keymap> keymap{std::make_shared<mtd::StubKeymap>()};
    MirKeyboardConfig const kbd_config{std::move(keymap)};
    MirInputDevice conf(MirInputDeviceId{3}, mi::DeviceCapability::keyboard, "keyboard", "keyboard-event3c");
    conf.set_keyboard_config(kbd_config);
    EXPECT_NO_THROW(mi::DefaultDevice dev(conf, queue, keyboard, mt::fake_shared(key_mapper), change_callback));
}
