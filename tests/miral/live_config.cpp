/*
 * Copyright © Canonical Ltd.
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
 */

#include <miral/live_config.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

TEST(LiveConfig, key_cannot_be_empty)
{
    EXPECT_THAT([] { miral::live_config::Key{}; },
        ThrowsMessage<std::invalid_argument>("Invalid (empty) live config key"));
}

TEST(LiveConfig, key_cannot_contain_empty_element)
{
    EXPECT_THAT([] { miral::live_config::Key{""}; },
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { miral::live_config::Key{"", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", ""}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));
}

TEST(LiveConfig, key_cannot_contain_an_element_starting_with_a_digit)
{
    EXPECT_THAT([] { miral::live_config::Key{"1"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 1"));

    EXPECT_THAT(([] { miral::live_config::Key{"0oo", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 0oo"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "2ar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 2ar"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "9az"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 9az"));
}

TEST(LiveConfig, key_cannot_contain_an_element_starting_with_underscore)
{
    EXPECT_THAT([] { miral::live_config::Key{"_"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _"));

    EXPECT_THAT(([] { miral::live_config::Key{"_oo", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _oo"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "_ar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _ar"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "_az"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _az"));
}

TEST(LiveConfig, key_cannot_contain_an_element_containing_uppercase)
{
    EXPECT_THAT([] { miral::live_config::Key{"Foo"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: Foo"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bAr", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: bAr"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "baZ"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baZ"));
}

TEST(LiveConfig, key_cannot_contain_an_element_containing_whitespaces_or_punctuation)
{
    EXPECT_THAT([] { miral::live_config::Key{"foo bar baz"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: foo bar baz"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar\n", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "baz%"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz%"));

    EXPECT_THAT([] { miral::live_config::Key{"foo.bar.baz"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: foo.bar.baz"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "baz!"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz!"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar/", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: bar/"));

    EXPECT_THAT(([] { miral::live_config::Key{"foo", "bar", "baz*"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz*"));
}

TEST(LiveConfig, key_can_contain_elements_with_digit_and_underscores)
{
    miral::live_config::Key{"foo_bar_baz"};

    miral::live_config::Key{"foo1", "bar2", "baz3"};

    miral::live_config::Key{"foo_122", "bar_", "baz"};

    miral::live_config::Key{"foo", "bar", "baz9"};
}