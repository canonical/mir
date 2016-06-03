/*
 * Copyright © 2013,2016 Canonical Ltd.
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
 * Authored by:
 *   Robert Carr <robert.carr@canonical.com>
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/xkb_mapper.h"
#include "mir/input/keymap.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include "mir/test/event_matchers.h"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <linux/input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mircv = mi::receiver;
namespace mev = mir::events;

using namespace ::testing;

struct XKBMapper : Test
{
    mircv::XKBMapper mapper;

    int map_key(MirKeyboardAction action, int scan_code)
    {
        auto ev = mev::make_event(MirInputDeviceId(0), std::chrono::nanoseconds(0), std::vector<uint8_t>{}, action,
                                  0, scan_code, mir_input_event_modifier_none);

        mapper.map_event(*ev);
        return ev->to_input()->to_keyboard()->key_code();
    }

    int map_key(MirInputDeviceId id, MirKeyboardAction action, int scan_code)
    {
        auto ev = mev::make_event(id, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, action, 0, scan_code,
                                  mir_input_event_modifier_none);

        mapper.map_event(*ev);
        return ev->to_input()->to_keyboard()->key_code();
    }

    MOCK_METHOD1(mapped_event,void(MirEvent const& ev));

    void map_event(MirInputDeviceId id, MirKeyboardAction action, int scan_code)
    {
        auto ev = mev::make_event(id, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, action, 0, scan_code,
                                  mir_input_event_modifier_none);

        mapper.map_event(*ev);
        mapped_event(*ev);
    }

    void map_pointer_event(MirInputDeviceId id,
                           MirPointerAction action,
                           MirPointerButtons buttons,
                           float relx,
                           float rely,
                           float horizontal_scroll = 0.0f,
                           float vertical_scroll = 0.0f)
    {
        auto ev =
            mev::make_event(id, std::chrono::nanoseconds{0}, std::vector<uint8_t>{}, mir_input_event_modifier_none,
                            action, buttons, 0.0f, 0.0f, horizontal_scroll, vertical_scroll, relx, rely);
        mapper.map_event(*ev);
        mapped_event(*ev);
    }
};

TEST_F(XKBMapper, maps_nothing_by_default)
{
    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_LEFTSHIFT));
}

TEST_F(XKBMapper, when_device_keymap_is_set_maps_generic_us_english_keys)
{
    mapper.set_keymap(MirInputDeviceId{0}, mi::Keymap{});

    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_down, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_up, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_up, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
}

TEST_F(XKBMapper, when_surface_keymap_is_set_maps_generic_us_english_keys)
{
    mapper.set_keymap(mi::Keymap{});

    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_down, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_up, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_up, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
}

TEST_F(XKBMapper, key_repeats_do_not_recurse_modifier_state)
{
    mapper.set_keymap(MirInputDeviceId{0}, mi::Keymap{});

    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_repeat, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_ampersand, map_key(mir_keyboard_action_down, KEY_7));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_7, map_key(mir_keyboard_action_down, KEY_7));
}

TEST_F(XKBMapper, default_keymap_replaces_device_keymap)
{
    mapper.set_keymap(MirInputDeviceId{0}, mi::Keymap{});
    mapper.set_keymap(mi::Keymap{"pc105","dvorak","",""});

    EXPECT_EQ(XKB_KEY_b, map_key(mir_keyboard_action_down, KEY_N));
    EXPECT_EQ(XKB_KEY_e, map_key(mir_keyboard_action_down, KEY_D));
}

TEST_F(XKBMapper, when_default_keymap_is_reset_no_key_is_mapped)
{
    mapper.set_keymap(mi::Keymap{"pc105","dvorak","",""});
    mapper.set_keymap(MirInputDeviceId{0}, mi::Keymap{});
    mapper.reset_keymap();

    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_N));
    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_D));
}

TEST_F(XKBMapper, keymapper_uses_device_id_to_pick_keymap)
{
    auto keyboard_us = MirInputDeviceId{3};
    auto keyboard_de = MirInputDeviceId{1};
    mapper.set_keymap(keyboard_us, mi::Keymap{});
    mapper.set_keymap(keyboard_de, mi::Keymap{"pc105", "de", "",""});

    EXPECT_EQ(XKB_KEY_y, map_key(keyboard_us, mir_keyboard_action_down, KEY_Y));
    EXPECT_EQ(XKB_KEY_z, map_key(keyboard_de, mir_keyboard_action_down, KEY_Y));
    EXPECT_EQ(XKB_KEY_z, map_key(keyboard_us, mir_keyboard_action_down, KEY_Z));
    EXPECT_EQ(XKB_KEY_y, map_key(keyboard_de, mir_keyboard_action_down, KEY_Z));
}

TEST_F(XKBMapper, key_strokes_of_modifier_key_update_modifier)
{
    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    auto keyboard_us = MirInputDeviceId{0};
    mapper.set_keymap(keyboard_us, mi::Keymap{});
    map_event(keyboard_us, mir_keyboard_action_down, KEY_LEFTSHIFT);
    map_event(keyboard_us, mir_keyboard_action_up, KEY_LEFTSHIFT);
}

TEST_F(XKBMapper, maps_modifier_keys_according_to_keymap)
{
    auto keyboard_us = MirInputDeviceId{0};
    auto keyboard_de = MirInputDeviceId{1};
    auto const no_mod = MirInputEventModifiers{mir_input_event_modifier_none};
    auto const right_alt = MirInputEventModifiers{mir_input_event_modifier_alt | mir_input_event_modifier_alt_right};
    mapper.set_keymap(keyboard_de, mi::Keymap{"pc105", "de", "",""});
    mapper.set_keymap(keyboard_us, mi::Keymap{});

    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(no_mod), mt::KeyOfSymbol(XKB_KEY_ISO_Level3_Shift))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(no_mod), mt::KeyOfSymbol(XKB_KEY_at))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(right_alt), mt::KeyOfSymbol(XKB_KEY_Alt_R))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(right_alt), mt::KeyOfSymbol(XKB_KEY_q))));

    map_event(keyboard_de, mir_keyboard_action_down, KEY_RIGHTALT);
    map_event(keyboard_de, mir_keyboard_action_down, KEY_Q);
    map_event(keyboard_us, mir_keyboard_action_down, KEY_RIGHTALT);
    map_event(keyboard_us, mir_keyboard_action_down, KEY_Q);
}


TEST_F(XKBMapper, modifier_events_on_different_keyboards_do_not_change_modifier_state)
{
    auto keyboard_us_1 = MirInputDeviceId{0};
    auto keyboard_us_2 = MirInputDeviceId{1};
    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(*this, mapped_event(AllOf(
                mt::KeyOfSymbol(XKB_KEY_a),
                mt::KeyWithModifiers(mir_input_event_modifier_none))));

    mapper.set_keymap(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap(keyboard_us_2, mi::Keymap{});
    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_LEFTSHIFT);
    map_event(keyboard_us_2, mir_keyboard_action_down, KEY_A);
}

TEST_F(XKBMapper, modifier_events_on_different_keyboards_contribute_to_pointer_event_modifier_state)
{
    auto keyboard_us_1 = MirInputDeviceId{0};
    auto keyboard_us_2 = MirInputDeviceId{1};
    auto pointer_device_id = MirInputDeviceId{2};
    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    const MirInputEventModifiers shift_right = mir_input_event_modifier_shift_right | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(shift_right)));
    EXPECT_CALL(*this, mapped_event(mt::PointerEventWithModifiers(shift_right | r_alt_modifier)));

    mapper.set_keymap(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap(keyboard_us_2, mi::Keymap{});
    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_RIGHTALT);
    map_event(keyboard_us_2, mir_keyboard_action_down, KEY_RIGHTSHIFT);
    map_pointer_event(pointer_device_id, mir_pointer_action_motion, 0, 12, 40);
}

TEST_F(XKBMapper, device_removal_removes_modifier_flags)
{
    auto keyboard_us_1 = MirInputDeviceId{0};
    auto keyboard_us_2 = MirInputDeviceId{1};
    auto pointer_device_id = MirInputDeviceId{2};
    const MirInputEventModifiers r_alt_modifier = mir_input_event_modifier_alt_right | mir_input_event_modifier_alt;
    const MirInputEventModifiers shift_right = mir_input_event_modifier_shift_right | mir_input_event_modifier_shift;
    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(r_alt_modifier)));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(shift_right)));
    EXPECT_CALL(*this, mapped_event(mt::PointerEventWithModifiers(r_alt_modifier)));

    mapper.set_keymap(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap(keyboard_us_2, mi::Keymap{});
    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_RIGHTALT);
    map_event(keyboard_us_2, mir_keyboard_action_down, KEY_RIGHTSHIFT);
    mapper.reset_keymap(keyboard_us_2);
    map_pointer_event(pointer_device_id, mir_pointer_action_motion, 0, 12, 40);
}

TEST_F(XKBMapper, when_single_keymap_is_used_key_states_are_still_tracked_separately)
{
    auto keyboard_us_1 = MirInputDeviceId{0};
    auto keyboard_us_2 = MirInputDeviceId{1};
    mapper.set_keymap(mi::Keymap{});

    const MirInputEventModifiers shift_left = mir_input_event_modifier_shift_left | mir_input_event_modifier_shift;

    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(shift_left)));
    EXPECT_CALL(*this, mapped_event(mt::KeyOfSymbol(XKB_KEY_a)));
    EXPECT_CALL(*this, mapped_event(mt::KeyOfSymbol(XKB_KEY_B)));

    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_LEFTSHIFT);
    map_event(keyboard_us_2, mir_keyboard_action_down, KEY_A);
    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_B);
}

TEST_F(XKBMapper, injected_key_state_affects_mapping_of_key_code)
{
    const MirInputEventModifiers no_modifier = mir_input_event_modifier_none;
    auto keyboard = MirInputDeviceId{0};
    mapper.set_keymap(keyboard, mi::Keymap{"pc105", "de", "",""});

    mapper.set_key_state(keyboard, {KEY_RIGHTALT});

    InSequence seq;
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(no_modifier),mt::KeyOfSymbol(XKB_KEY_at))));

    map_event(keyboard, mir_keyboard_action_down, KEY_Q);
}
