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
    inline static std::initializer_list<std::pair<XkbSymkey, Action>> const wasd_key_action = {
        std::pair{XKB_KEY_w, Action::move_up},
        std::pair{XKB_KEY_a, Action::move_left},
        std::pair{XKB_KEY_s, Action::move_down},
        std::pair{XKB_KEY_d, Action::move_right}};
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
    auto const keymap = mi::MouseKeysKeymap(wasd_key_action);

    keymap.for_each_key_action_pair(
        [&](auto const key, auto const action)
        {
            auto const iter = std::ranges::find(
                    wasd_key_action,
                    key,
                    [](auto const key_action)
                    {
                        return key_action.first;
                    });

            ASSERT_TRUE(iter != key_action_pairs.end());
            ASSERT_EQ(action, iter->second);
        });
}

struct TestMouseKeysKeymapGetAction :
    public TestMouseKeysKeymap,
    public testing::WithParamInterface<std::pair<XkbSymkey, Action>>
{
};

TEST_P(TestMouseKeysKeymapGetAction, get_action_returns_the_appropriate_action)
{
    auto [symkey, action] = GetParam();
    auto const keymap = mi::MouseKeysKeymap{{symkey, action}};
    ASSERT_EQ(keymap.get_action(symkey), action);
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysKeymap, TestMouseKeysKeymapGetAction, testing::ValuesIn(TestMouseKeysKeymap::wasd_key_action));

struct TestMouseKeysKeymapSetAction :
    public TestMouseKeysKeymap,
    public testing::WithParamInterface<std::pair<XkbSymkey, Action>>
{
};

TEST_P(TestMouseKeysKeymapSetAction, set_action_sets_the_appropriate_action_and_clears_it_correctly)
{
    auto const [key, action] = GetParam();
    auto keymap = mi::MouseKeysKeymap{};

    keymap.set_action(key,action);
    ASSERT_EQ(keymap.get_action(key), action);

    keymap.set_action(key, {});
    ASSERT_EQ(keymap.get_action(key), std::nullopt);
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysKeymap, TestMouseKeysKeymapSetAction, testing::ValuesIn(TestMouseKeysKeymap::wasd_key_action));
