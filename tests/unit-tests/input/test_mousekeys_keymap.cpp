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

#include "mir/input/mousekeys_keymap.h"

#include <gtest/gtest.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mi = mir::input;
using Action = mi::MouseKeysKeymap::Action;
using XkbSymkey = mi::XkbSymkey;

struct TestMouseKeysKeymap : testing::Test
{
};

TEST_F(TestMouseKeysKeymap, default_keymaps_have_no_key_action_pairs)
{
    auto num_pairs = 0;
    mi::MouseKeysKeymap const keymap;
    keymap.for_each_key_action_pair(
        [&](auto, auto)
        {
            num_pairs += 1;
        });

    ASSERT_EQ(num_pairs, 0);
}

TEST_F(TestMouseKeysKeymap, initializer_list_constructor_add_passed_key_action_pairs)
{
    auto const key_action_pairs = {
        std::pair<XkbSymkey, Action>{XKB_KEY_w, Action::move_up},
        {XKB_KEY_a, Action::move_left},
        {XKB_KEY_s, Action::move_down},
        {XKB_KEY_d, Action::move_right}};

    auto const keymap = mi::MouseKeysKeymap(key_action_pairs);

    keymap.for_each_key_action_pair(
        [&](auto const key, auto const action)
        {
            auto const iter = std::ranges::find(
                    key_action_pairs,
                    key,
                    [](auto const key_action)
                    {
                        return key_action.first;
                    });

            ASSERT_TRUE(iter != key_action_pairs.end());
            ASSERT_EQ(action, iter->second);
        });
}

TEST_F(TestMouseKeysKeymap, get_action_returns_the_appropriate_action)
{
    auto const key_action_pairs = {
        std::pair<XkbSymkey, Action>{XKB_KEY_w, Action::move_up},
        {XKB_KEY_a, Action::move_left},
        {XKB_KEY_s, Action::move_down},
        {XKB_KEY_d, Action::move_right}};

    auto const keymap = mi::MouseKeysKeymap{key_action_pairs};
    for(auto const& [key, action]: key_action_pairs)
    {
        ASSERT_EQ(keymap.get_action(key), action);
    }
}

TEST_F(TestMouseKeysKeymap, set_action_sets_the_appropriate_action_and_clears_it_correctly)
{
    auto const key_action_pairs = {
        std::pair<XkbSymkey, Action>{XKB_KEY_w, Action::move_up},
        {XKB_KEY_a, Action::move_left},
        {XKB_KEY_s, Action::move_down},
        {XKB_KEY_d, Action::move_right}};

    auto keymap = mi::MouseKeysKeymap{};
    for(auto const& [key, action]: key_action_pairs)
    {
        keymap.set_action(key, action);
        ASSERT_EQ(keymap.get_action(key), action);

        keymap.set_action(key, {});
        ASSERT_EQ(keymap.get_action(key), std::nullopt);
    }
}
