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
namespace mlc = miral::live_config;

TEST(LiveConfig, key_cannot_be_empty)
{
    EXPECT_THAT([] { mlc::Key{}; },
        ThrowsMessage<std::invalid_argument>("Invalid (empty) live config key"));
}

TEST(LiveConfig, key_cannot_contain_empty_element)
{
    EXPECT_THAT([] { mlc::Key{""}; },
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { mlc::Key{"", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { mlc::Key{"foo", "", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", ""}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));
}

TEST(LiveConfig, key_cannot_contain_an_element_starting_with_a_digit)
{
    EXPECT_THAT([] { mlc::Key{"1"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 1"));

    EXPECT_THAT(([] { mlc::Key{"0oo", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 0oo"));

    EXPECT_THAT(([] { mlc::Key{"foo", "2ar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 2ar"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "9az"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: 9az"));
}

TEST(LiveConfig, key_cannot_contain_an_element_starting_with_underscore)
{
    EXPECT_THAT([] { mlc::Key{"_"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _"));

    EXPECT_THAT(([] { mlc::Key{"_oo", "bar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _oo"));

    EXPECT_THAT(([] { mlc::Key{"foo", "_ar", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _ar"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "_az"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: _az"));
}

TEST(LiveConfig, key_cannot_contain_an_element_containing_uppercase)
{
    EXPECT_THAT([] { mlc::Key{"Foo"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: Foo"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bAr", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: bAr"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "baZ"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baZ"));
}

TEST(LiveConfig, key_cannot_contain_an_element_containing_whitespaces_or_punctuation)
{
    EXPECT_THAT([] { mlc::Key{"foo bar baz"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: foo bar baz"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar\n", "baz"}; }),
        ThrowsMessage<std::invalid_argument>(HasSubstr("Invalid config key element:")));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "baz%"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz%"));

    EXPECT_THAT([] { mlc::Key{"foo.bar.baz"}; },
        ThrowsMessage<std::invalid_argument>("Invalid config key element: foo.bar.baz"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "baz!"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz!"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar/", "baz"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: bar/"));

    EXPECT_THAT(([] { mlc::Key{"foo", "bar", "baz*"}; }),
        ThrowsMessage<std::invalid_argument>("Invalid config key element: baz*"));
}

TEST(LiveConfig, key_can_contain_elements_with_digit_and_underscores)
{
    mlc::Key{"foo_bar_baz"};

    mlc::Key{"foo1", "bar2", "baz3"};

    mlc::Key{"foo_122", "bar_", "baz"};

    mlc::Key{"foo", "bar", "baz9"};
}

TEST(LiveConfig, key_has_expected_representations)
{
    mlc::Key const foo_bar_baz{"foo_bar_baz"};
    EXPECT_THAT(foo_bar_baz.to_string(), Eq("foo_bar_baz"));
    foo_bar_baz.with_key([&](std::vector<std::string> const& key)
    {
        EXPECT_THAT(key, ElementsAre("foo_bar_baz"));
    });

    mlc::Key const foo1_bar2_baz3{"foo1", "bar2", "baz3"};
    EXPECT_THAT(foo1_bar2_baz3.to_string(), Eq("foo1_bar2_baz3"));
    foo1_bar2_baz3.with_key([&](std::vector<std::string> const& key)
    {
        EXPECT_THAT(key, ElementsAre("foo1", "bar2", "baz3"));
    });

    mlc::Key const foo_122_bar__baz{"foo_122", "bar_", "baz"};
    EXPECT_THAT(foo_122_bar__baz.to_string(), Eq("foo_122_bar__baz"));
    foo_122_bar__baz.with_key([&](std::vector<std::string> const& key)
    {
        EXPECT_THAT(key, ElementsAre("foo_122", "bar_", "baz"));
    });
}