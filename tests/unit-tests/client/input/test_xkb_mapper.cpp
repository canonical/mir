/*
 * Copyright © 2013, 2016 Canonical Ltd.
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
 * Authored by:
 *   Robert Carr <robert.carr@canonical.com>
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/xkb_mapper.h"
#include "mir/input/keymap.h"
#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include "mir_test_framework/temporary_environment_value.h"
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
namespace mtf = mir_test_framework;

using namespace ::testing;

struct XKBMapper : Test
{
    // en_US.UTF-8 is the fallback used by libxkbcommon - setting this avoids
    // tests failing on systems with a locale that points to a working but 
    // different set of compose sequences
    mir_test_framework::TemporaryEnvironmentValue lc_all{"LC_ALL", "en_US.UTF-8"};
    int const xkb_new_unicode_symbols_offset = 0x1000000;
    mircv::XKBMapper mapper;

    int map_key(mircv::XKBMapper &keymapper, MirInputDeviceId dev, MirKeyboardAction action, int scan_code)
    {
        auto ev = mev::make_event(dev, std::chrono::nanoseconds(0), std::vector<uint8_t>{}, action,
                                  0, scan_code, mir_input_event_modifier_none);

        keymapper.map_event(*ev);
        return ev->to_input()->to_keyboard()->key_code();
    }

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
    mapper.set_keymap_for_device(MirInputDeviceId{0}, mi::Keymap{});

    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_down, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_up, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_up, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));

    EXPECT_EQ(XKB_KEY_Super_L, map_key(mir_keyboard_action_down, KEY_LEFTMETA));
    EXPECT_EQ(XKB_KEY_Super_R, map_key(mir_keyboard_action_down, KEY_RIGHTMETA));
}

TEST_F(XKBMapper, when_surface_keymap_is_set_maps_generic_us_english_keys)
{
    mapper.set_keymap_for_all_devices(mi::Keymap{});

    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_down, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_down, KEY_4));
    EXPECT_EQ(XKB_KEY_dollar, map_key(mir_keyboard_action_up, KEY_4));
    EXPECT_EQ(XKB_KEY_Shift_L, map_key(mir_keyboard_action_up, KEY_LEFTSHIFT));
    EXPECT_EQ(XKB_KEY_4, map_key(mir_keyboard_action_down, KEY_4));

    EXPECT_EQ(XKB_KEY_Super_L, map_key(mir_keyboard_action_down, KEY_LEFTMETA));
    EXPECT_EQ(XKB_KEY_Super_R, map_key(mir_keyboard_action_down, KEY_RIGHTMETA));
}

TEST_F(XKBMapper, key_repeats_do_not_recurse_modifier_state)
{
    mapper.set_keymap_for_device(MirInputDeviceId{0}, mi::Keymap{});

    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_repeat, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_ampersand, map_key(mir_keyboard_action_down, KEY_7));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_7, map_key(mir_keyboard_action_down, KEY_7));
}

TEST_F(XKBMapper, keymap_for_all_devices_replace_device_keymap)
{
    mapper.set_keymap_for_device(MirInputDeviceId{0}, mi::Keymap{});
    mapper.set_keymap_for_all_devices(mi::Keymap{"pc105","dvorak","",""});

    EXPECT_EQ(XKB_KEY_b, map_key(mir_keyboard_action_down, KEY_N));
    EXPECT_EQ(XKB_KEY_e, map_key(mir_keyboard_action_down, KEY_D));
}

TEST_F(XKBMapper, when_keymap_for_all_devices_is_reset_no_key_is_mapped)
{
    mapper.set_keymap_for_all_devices(mi::Keymap{"pc105","dvorak","",""});
    mapper.set_keymap_for_device(MirInputDeviceId{0}, mi::Keymap{});
    mapper.clear_all_keymaps();

    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_N));
    EXPECT_EQ(0, map_key(mir_keyboard_action_down, KEY_D));
}

TEST_F(XKBMapper, keymapper_uses_device_id_to_pick_keymap)
{
    auto keyboard_us = MirInputDeviceId{3};
    auto keyboard_de = MirInputDeviceId{1};
    mapper.set_keymap_for_device(keyboard_us, mi::Keymap{});
    mapper.set_keymap_for_device(keyboard_de, mi::Keymap{"pc105", "de", "",""});

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
    mapper.set_keymap_for_device(keyboard_us, mi::Keymap{});
    map_event(keyboard_us, mir_keyboard_action_down, KEY_LEFTSHIFT);
    map_event(keyboard_us, mir_keyboard_action_up, KEY_LEFTSHIFT);
}

TEST_F(XKBMapper, maps_kernel_meta_to_mir_meta)  // AKA "Super"
{   // Regression test for LP: #1602966
    const MirInputEventModifiers meta_left =
        mir_input_event_modifier_meta_left | mir_input_event_modifier_meta;
    const MirInputEventModifiers meta_right =
        mir_input_event_modifier_meta_right | mir_input_event_modifier_meta;

    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(meta_left)));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(meta_right)));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithModifiers(mir_input_event_modifier_none)));

    auto keyboard_us = MirInputDeviceId{0};
    mapper.set_keymap_for_device(keyboard_us, mi::Keymap{});
    map_event(keyboard_us, mir_keyboard_action_down, KEY_LEFTMETA);
    map_event(keyboard_us, mir_keyboard_action_up, KEY_LEFTMETA);
    map_event(keyboard_us, mir_keyboard_action_down, KEY_RIGHTMETA);
    map_event(keyboard_us, mir_keyboard_action_up, KEY_RIGHTMETA);
}

TEST_F(XKBMapper, maps_modifier_keys_according_to_keymap)
{
    auto keyboard_us = MirInputDeviceId{0};
    auto keyboard_de = MirInputDeviceId{1};
    auto const no_mod = MirInputEventModifiers{mir_input_event_modifier_none};
    auto const right_alt = MirInputEventModifiers{mir_input_event_modifier_alt | mir_input_event_modifier_alt_right};
    mapper.set_keymap_for_device(keyboard_de, mi::Keymap{"pc105", "de", "",""});
    mapper.set_keymap_for_device(keyboard_us, mi::Keymap{});

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

    mapper.set_keymap_for_device(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap_for_device(keyboard_us_2, mi::Keymap{});
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

    mapper.set_keymap_for_device(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap_for_device(keyboard_us_2, mi::Keymap{});
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

    mapper.set_keymap_for_device(keyboard_us_1, mi::Keymap{});
    mapper.set_keymap_for_device(keyboard_us_2, mi::Keymap{});
    map_event(keyboard_us_1, mir_keyboard_action_down, KEY_RIGHTALT);
    map_event(keyboard_us_2, mir_keyboard_action_down, KEY_RIGHTSHIFT);
    mapper.clear_keymap_for_device(keyboard_us_2);
    map_pointer_event(pointer_device_id, mir_pointer_action_motion, 0, 12, 40);
}

TEST_F(XKBMapper, when_single_keymap_is_used_key_states_are_still_tracked_separately)
{
    auto keyboard_us_1 = MirInputDeviceId{0};
    auto keyboard_us_2 = MirInputDeviceId{1};
    mapper.set_keymap_for_all_devices(mi::Keymap{});

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
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "de", "",""});

    mapper.set_key_state(keyboard, {KEY_RIGHTALT});

    InSequence seq;
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithModifiers(no_modifier),mt::KeyOfSymbol(XKB_KEY_at))));

    map_event(keyboard, mir_keyboard_action_down, KEY_Q);
}

TEST_F(XKBMapper, on_czech_qwerty_caps_lock_should_provide_uppercase_letters)
{
    auto keyboard_cz = MirInputDeviceId{0};
    mapper.set_keymap_for_all_devices(mi::Keymap{"pc105+inet", "cz", "qwerty", ""});

    InSequence seq;
    EXPECT_CALL(*this, mapped_event(mt::KeyOfSymbol(XKB_KEY_Caps_Lock)));
    EXPECT_CALL(*this, mapped_event(mt::KeyOfSymbol(XKB_KEY_Caps_Lock)));
    EXPECT_CALL(*this, mapped_event(mt::KeyOfSymbol(XKB_KEY_Ecaron)));

    map_event(keyboard_cz, mir_keyboard_action_down, KEY_CAPSLOCK);
    map_event(keyboard_cz, mir_keyboard_action_up, KEY_CAPSLOCK);
    map_event(keyboard_cz, mir_keyboard_action_down, KEY_2);
}

TEST_F(XKBMapper, setting_keymap_again_overwrites_keymap)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "",""});
    EXPECT_EQ(XKB_KEY_y, map_key(keyboard, mir_keyboard_action_down, KEY_Y));
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "de", "",""});
    EXPECT_EQ(XKB_KEY_y, map_key(keyboard, mir_keyboard_action_down, KEY_Z));
}

TEST_F(XKBMapper, composes_keys_on_deadkey_keymap)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl",""});
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_down, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_up, KEY_U));
}

TEST_F(XKBMapper, release_of_compose_sequence_keys_after_completion_still_yields_dead_keys)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl",""});
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Udiaeresis, map_key(keyboard, mir_keyboard_action_down, KEY_U));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Udiaeresis, map_key(keyboard, mir_keyboard_action_up, KEY_U));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));
}

TEST_F(XKBMapper, repeated_key_that_completed_the_sequence_yields_a_repeated_completion_key)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl",""});
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_down, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
    EXPECT_EQ(XKB_KEY_udiaeresis, map_key(keyboard, mir_keyboard_action_repeat, KEY_U));
}

TEST_F(XKBMapper, compose_key_option_activates_multi_key_based_sequences)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl","compose:ralt"});
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTALT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTALT));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_COMMA));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_COMMA));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));
    auto const black_heart = xkb_new_unicode_symbols_offset + 0x2665;
    EXPECT_EQ(black_heart, map_key(keyboard, mir_keyboard_action_down, KEY_3));
}

TEST_F(XKBMapper, breaking_a_compose_sequence_yields_no_keysym)
{
    auto keyboard = MirInputDeviceId{3};
    mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl",""});
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));

    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_down, KEY_N));
    EXPECT_EQ(XKB_KEY_NoSymbol, map_key(keyboard, mir_keyboard_action_up, KEY_N));

    EXPECT_EQ(XKB_KEY_u, map_key(keyboard, mir_keyboard_action_down, KEY_U));
    EXPECT_EQ(XKB_KEY_u, map_key(keyboard, mir_keyboard_action_up, KEY_U));
}

TEST_F(XKBMapper, no_key_composition_when_no_compose_file_is_found)
{
    mtf::TemporaryEnvironmentValue compose_file_path{"XCOMPOSEFILE", "/foo/bogus/path/Compose"};
    mtf::TemporaryEnvironmentValue home_path{"HOME", "/not/your/home"};
    mtf::TemporaryEnvironmentValue localedir{"XLOCALEDIR", "/not/your/home"};
    mircv::XKBMapper local_mapper;

    auto keyboard = MirInputDeviceId{3};
    local_mapper.set_keymap_for_device(keyboard, mi::Keymap{"pc105", "us", "intl",""});
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(local_mapper, keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_dead_diaeresis, map_key(local_mapper, keyboard, mir_keyboard_action_down, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_dead_diaeresis, map_key(local_mapper, keyboard, mir_keyboard_action_up, KEY_APOSTROPHE));
    EXPECT_EQ(XKB_KEY_Shift_R, map_key(local_mapper, keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT));
    EXPECT_EQ(XKB_KEY_u, map_key(local_mapper, keyboard, mir_keyboard_action_down, KEY_U));
    EXPECT_EQ(XKB_KEY_u, map_key(local_mapper, keyboard, mir_keyboard_action_up, KEY_U));
}

namespace
{
struct XKBMapperWithUsKeymap : XKBMapper
{
    MirInputDeviceId keyboard = MirInputDeviceId{0};
    XKBMapperWithUsKeymap()
    {
        mapper.set_keymap_for_all_devices(mi::Keymap{"pc105", "us", "intl",""});
    }
};
}

TEST_F(XKBMapperWithUsKeymap, key_event_contains_text_to_append)
{
    EXPECT_CALL(*this, mapped_event(mt::KeyWithText("u")));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithText("")));

    map_event(keyboard, mir_keyboard_action_down, KEY_U);
    map_event(keyboard, mir_keyboard_action_up, KEY_U);
}

TEST_F(XKBMapperWithUsKeymap, key_event_text_applies_shift_modifiers)
{
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_Shift_L))));
    EXPECT_CALL(*this, mapped_event(mt::KeyWithText("U")));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_U))));

    map_event(keyboard, mir_keyboard_action_down, KEY_LEFTSHIFT);
    map_event(keyboard, mir_keyboard_action_down, KEY_U);
    map_event(keyboard, mir_keyboard_action_up, KEY_U);
}

TEST_F(XKBMapperWithUsKeymap, on_ctrl_c_key_event_text_is_u0003)
{
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_Control_R))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText("\003"), mt::KeyOfSymbol(XKB_KEY_c))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""), mt::KeyOfSymbol(XKB_KEY_c))));

    map_event(keyboard, mir_keyboard_action_down, KEY_RIGHTCTRL);
    map_event(keyboard, mir_keyboard_action_down, KEY_C);
    map_event(keyboard, mir_keyboard_action_up, KEY_C);
}

TEST_F(XKBMapperWithUsKeymap, compose_key_sequence_contains_text_on_key_down)
{
    InSequence seq;
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""),mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""),mt::KeyOfSymbol(XKB_KEY_NoSymbol))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""),mt::KeyOfSymbol(XKB_KEY_NoSymbol))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""),mt::KeyOfSymbol(XKB_KEY_Shift_R))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText("ü"),mt::KeyOfSymbol(XKB_KEY_udiaeresis))));
    EXPECT_CALL(*this, mapped_event(AllOf(mt::KeyWithText(""),mt::KeyOfSymbol(XKB_KEY_udiaeresis))));

    map_event(keyboard, mir_keyboard_action_down, KEY_RIGHTSHIFT);
    map_event(keyboard, mir_keyboard_action_down, KEY_APOSTROPHE);
    map_event(keyboard, mir_keyboard_action_up, KEY_APOSTROPHE);
    map_event(keyboard, mir_keyboard_action_up, KEY_RIGHTSHIFT);
    map_event(keyboard, mir_keyboard_action_down, KEY_U);
    map_event(keyboard, mir_keyboard_action_up, KEY_U);
}
