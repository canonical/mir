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

#include "src/server/input/default_event_builder.h"

#include <mir/input/xkb_mapper_registrar.h>
#include <mir/input/parameter_keymap.h>
#include <mir/input/led_observer_registrar.h>
#include <mir/executor.h>
#include <mir/test/doubles/advanceable_clock.h>
#include <mir/test/fake_shared.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/input.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mi = mir::input;

using namespace ::testing;

namespace
{
struct MockLedObserver : mi::LedObserver
{
    MOCK_METHOD(void, leds_set, (mi::KeyboardLeds leds), (override));
};

struct XKBMapperRegistrarLeds : Test
{
    void press_and_release(int scan_code, xkb_keysym_t keysym)
    {
        auto down = builder.key_event(std::nullopt, mir_keyboard_action_down, keysym, scan_code);
        mapper.map_event(*down);
        auto up = builder.key_event(std::nullopt, mir_keyboard_action_up, keysym, scan_code);
        mapper.map_event(*up);
    }

    static auto keymap(std::string layout) -> std::shared_ptr<mi::Keymap>
    {
        return std::make_shared<mi::ParameterKeymap>("pc105", std::move(layout), "", "");
    }

    MirInputDeviceId const device_id{17};
    mtd::AdvanceableClock clock;
    mi::DefaultEventBuilder builder{device_id, mt::fake_shared(clock)};
    mi::receiver::XKBMapperRegistrar mapper{mir::immediate_executor};
    std::shared_ptr<NiceMock<MockLedObserver>> const observer{std::make_shared<NiceMock<MockLedObserver>>()};
};
}

TEST_F(XKBMapperRegistrarLeds, caps_lock_sets_caps_lock_led)
{
    mapper.set_keymap_for_device(device_id, keymap("us"));
    mapper.register_interest(observer, device_id);

    EXPECT_CALL(*observer, leds_set(mi::KeyboardLeds{mi::KeyboardLed::caps_lock}));

    press_and_release(KEY_CAPSLOCK, XKB_KEY_Caps_Lock);
}

TEST_F(XKBMapperRegistrarLeds, leds_still_update_after_keymap_is_replaced)
{
    mapper.set_keymap_for_device(device_id, keymap("us"));
    mapper.register_interest(observer, device_id);

    mapper.set_keymap_for_device(device_id, keymap("latam"));

    EXPECT_CALL(*observer, leds_set(mi::KeyboardLeds{mi::KeyboardLed::caps_lock}));

    press_and_release(KEY_CAPSLOCK, XKB_KEY_Caps_Lock);
}

TEST_F(XKBMapperRegistrarLeds, leds_still_update_after_default_keymap_is_replaced)
{
    mapper.set_keymap_for_all_devices(keymap("us"));
    mapper.register_interest(observer, device_id);

    mapper.set_keymap_for_all_devices(keymap("latam"));

    EXPECT_CALL(*observer, leds_set(mi::KeyboardLeds{mi::KeyboardLed::num_lock}));

    press_and_release(KEY_NUMLOCK, XKB_KEY_Num_Lock);
}

TEST_F(XKBMapperRegistrarLeds, leds_update_when_registered_before_any_keymap)
{
    mapper.register_interest(observer, device_id);
    mapper.set_keymap_for_device(device_id, keymap("us"));

    EXPECT_CALL(*observer, leds_set(mi::KeyboardLeds{mi::KeyboardLed::caps_lock}));

    press_and_release(KEY_CAPSLOCK, XKB_KEY_Caps_Lock);
}

TEST_F(XKBMapperRegistrarLeds, replacing_keymap_clears_leds)
{
    mapper.set_keymap_for_device(device_id, keymap("us"));
    mapper.register_interest(observer, device_id);
    press_and_release(KEY_CAPSLOCK, XKB_KEY_Caps_Lock);

    EXPECT_CALL(*observer, leds_set(mi::KeyboardLeds{}));

    mapper.set_keymap_for_device(device_id, keymap("latam"));
}

TEST_F(XKBMapperRegistrarLeds, unregistered_observer_is_not_notified)
{
    mapper.set_keymap_for_device(device_id, keymap("us"));
    mapper.register_interest(observer, device_id);
    mapper.unregister_interest(*observer, device_id);

    EXPECT_CALL(*observer, leds_set(_)).Times(0);

    press_and_release(KEY_CAPSLOCK, XKB_KEY_Caps_Lock);
}
