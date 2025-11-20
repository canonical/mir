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

#include <mir/input/buffer_keymap.h>
#include <mir/input/parameter_keymap.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;

using namespace ::testing;

TEST(BufferKeymap, can_create_buffer_keymap)
{
    mi::BufferKeymap keymap{"test-keymap", {}, XKB_KEYMAP_FORMAT_TEXT_V1};
}

TEST(BufferKeymap, matches_matching_buffer_keymap)
{
    mi::BufferKeymap keymap_a{"test-keymap", {1, 2, 3}, XKB_KEYMAP_FORMAT_TEXT_V1};
    mi::BufferKeymap keymap_b{"test-keymap", {1, 2, 3}, XKB_KEYMAP_FORMAT_TEXT_V1};
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_a).matches(keymap_b), Eq(true));
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_b).matches(keymap_a), Eq(true));
}

TEST(BufferKeymap, different_buffers_fail_to_match)
{
    mi::BufferKeymap keymap_a{"test-keymap", {1, 2, 3}, XKB_KEYMAP_FORMAT_TEXT_V1};
    mi::BufferKeymap keymap_b{"test-keymap", {4, 5, 6}, XKB_KEYMAP_FORMAT_TEXT_V1};
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_a).matches(keymap_b), Eq(false));
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_b).matches(keymap_a), Eq(false));
}

TEST(BufferKeymap, different_formats_fail_to_match)
{
    mi::BufferKeymap keymap_a{"test-keymap", {1, 2, 3}, XKB_KEYMAP_FORMAT_TEXT_V1};
    mi::BufferKeymap keymap_b{"test-keymap", {1, 2, 3}, static_cast<xkb_keymap_format>(99)};
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_a).matches(keymap_b), Eq(false));
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_b).matches(keymap_a), Eq(false));
}

TEST(BufferKeymap, param_keymap_fail_to_match)
{
    mi::BufferKeymap keymap_a{"test-keymap", {1, 2, 3}, XKB_KEYMAP_FORMAT_TEXT_V1};
    mi::ParameterKeymap keymap_b{};
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_a).matches(keymap_b), Eq(false));
    EXPECT_THAT(static_cast<mi::Keymap&>(keymap_b).matches(keymap_a), Eq(false));
}
