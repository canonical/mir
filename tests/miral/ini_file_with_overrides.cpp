/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include <miral/live_config_ini_file_with_overrides.h>
#include <miral/live_config_overrides_list.h>
#include <live_config_overrides_list_builder.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace testing;
using namespace std::string_literals;
namespace mlc = miral::live_config;

struct IniFileWithOverridesTest : Test
{
    mlc::IniFileWithOverrides config_store;

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, float_handler, (mlc::Key const& key, std::optional<float> value));
    MOCK_METHOD(void, string_handler, (mlc::Key const& key, std::optional<std::string_view> value));
    MOCK_METHOD(void, strings_handler, (mlc::Key const& key, std::optional<std::span<std::string const>> value));
    MOCK_METHOD(void, ints_handler, (mlc::Key const& key, std::optional<std::span<int const>> value));

    mlc::Key const a_key{"a_scope", "an_int"};
    mlc::Key const another_key{"another_scope", "an_int"};
    mlc::Key const a_string_key{"a_string"};
    mlc::Key const a_strings_key{"strings"};
    mlc::Key const an_ints_key{"ints"};

    using FileContents = std::vector<std::pair<std::string, std::filesystem::path>>;
    void load(FileContents contents_and_paths)
    {
        mlc::OverridesListBuilder changes;
        for (auto const& [contents, path] : contents_and_paths)
            changes.push_modified(
                path,
                [s = contents]{ return std::make_unique<std::istringstream>(s); });

        config_store.load(changes.build());
    }
};

TEST_F(IniFileWithOverridesTest, later_file_overrides_earlier_scalar)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Optional(99)));

    load({
        {"a_scope_an_int=42\n", "base.ini"},
        {"a_scope_an_int=99\n", "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, array_values_are_appended_across_files)
{
    config_store.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("alpha", "beta", "gamma"))));

    load({
        {"strings=alpha\n", "file1.ini"},
        {"strings=beta\n", "file2.ini"},
        {"strings=gamma\n", "file3.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, combined_state_aggregated_across_files)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });
    config_store.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Optional(7)));
    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar", "baz", "qux"))));

    auto const base = R"(
        a_scope_an_int=3
        strings=foo
        strings=bar
    )";
    auto const override_file = R"(
        a_scope_an_int=7
        strings=baz
        strings=qux
    )";

    load({
        {base, "base.ini"},
        {override_file, "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, array_clear_in_later_file_removes_earlier_entries)
{
    config_store.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("only"))));

    auto const base = R"(
        strings=first
        strings=second
    )";
    auto const override_file = R"(
        strings=
        strings=only
    )";
    load({
        {base, "base.ini"},
        {override_file, "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, done_handler_fires_after_attribute_handlers)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });
    config_store.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    bool done_called = false;
    config_store.on_done([&] { done_called = true; });

    EXPECT_CALL(*this, int_handler(_, _)).WillRepeatedly([&](auto&&...) { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, strings_handler(_, _)).WillRepeatedly([&](auto&&...) { EXPECT_THAT(done_called, IsFalse()); });

    auto const base = R"(
        a_scope_an_int=1
        strings=hello
    )";
    load({{base, "base.ini"}});

    EXPECT_THAT(done_called, IsTrue());
}

TEST_F(IniFileWithOverridesTest, empty_value_in_later_file_then_more_entries_yields_only_later_entries)
{
    config_store.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("second_file", "third_file"))));

    auto const file1 = R"(
        strings=first_a
        strings=first_b
    )";
    auto const file2 = R"(
        strings=
        strings=second_file
    )";
    load({
        {file1, "file1.ini"},
        {file2, "file2.ini"},
        {"strings=third_file\n", "file3.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, handlers_called_exactly_once_per_load)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });
    config_store.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    int const num_loads = 4;
    EXPECT_CALL(*this, int_handler(a_key, _)).Times(num_loads);
    EXPECT_CALL(*this, ints_handler(an_ints_key, _)).Times(num_loads);

    auto const content = R"(
        a_scope_an_int=1
        ints=10
    )";
    for (int i = 0; i < num_loads; ++i)
        load({{content, "single.ini"}});
}

TEST_F(IniFileWithOverridesTest, done_handler_called_exactly_once_per_load)
{
    int done_count = 0;
    config_store.on_done([&] { ++done_count; });

    load({{"", "file1.ini"}});
    load({{"", "file2.ini"}});
    load({{"", "file3.ini"}});

    EXPECT_THAT(done_count, Eq(3));
}

TEST_F(IniFileWithOverridesTest, unset_key_yields_nullopt)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Eq(std::nullopt)));

    load({
        {"another_scope_an_int=10\n", "file1.ini"},
        {"another_scope_an_int=20\n", "file2.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, reload_updates_scalar_value)
{
    config_store.add_float_attribute(a_key, "a float", [this](auto... args) { float_handler(args...); });

    {
        InSequence seq;
        EXPECT_CALL(*this, float_handler(a_key, Optional(FloatEq(1.0f))));
        EXPECT_CALL(*this, float_handler(a_key, Optional(FloatEq(2.0f))));
    }

    load({{"a_scope_an_int=1.0\n", "base.ini"}});
    load({{"a_scope_an_int=2.0\n", "base.ini"}});
}

TEST_F(IniFileWithOverridesTest, later_file_wins_for_shared_key_across_reloads)
{
    mlc::Key const cursor_scale_key{"cursor", "scale"};
    mlc::Key const extra_key{"extra", "value"};

    config_store.add_float_attribute(
        cursor_scale_key, "cursor scale", [this](auto... args) { float_handler(args...); });
    config_store.add_float_attribute(extra_key, "extra value", [this](auto... args) { float_handler(args...); });

    {
        InSequence seq;
        EXPECT_CALL(*this, float_handler(cursor_scale_key, Optional(FloatEq(2.0f))));
        EXPECT_CALL(*this, float_handler(extra_key, Eq(std::nullopt)));

        EXPECT_CALL(*this, float_handler(cursor_scale_key, Optional(FloatEq(2.0f))));
        EXPECT_CALL(*this, float_handler(extra_key, Optional(FloatEq(3.5f))));
    }

    load({
        {"cursor_scale=1.0\n", "base.ini"},
        {"cursor_scale=2.0\n", "override.ini"},
    });

    auto const updated_base = R"(
        cursor_scale=1.0
        extra_value=3.5
    )";
    load({
        {updated_base, "base.ini"},
        {"cursor_scale=2.0\n", "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, invalid_scalar_value_in_later_file_yields_nullopt)
{
    config_store.add_int_attribute(a_key, "an int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Eq(std::nullopt)));

    load({
        {"a_scope_an_int=10\n", "base.ini"},
        {"a_scope_an_int=not_a_number\n", "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, invalid_array_value_in_later_file_is_ignored)
{
    config_store.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2, 3))));

    auto const base = R"(
        ints=1
        ints=2
    )";
    auto const override_file = R"(
        ints=3
        ints=not_a_number
    )";
    load({
        {base, "base.ini"},
        {override_file, "override.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, preset_scalar_used_when_all_files_omit_key)
{
    config_store.add_int_attribute(a_key, "an int", 5, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Optional(5)));

    load({
        {"another_scope_an_int=1\n", "file1.ini"},
        {"another_scope_an_int=2\n", "file2.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, preset_array_used_when_all_files_omit_key)
{
    std::vector<std::string> const preset_values{"default1"s, "default2"s};
    config_store.add_strings_attribute(
        a_strings_key,
        "strings",
        std::span<std::string const>{preset_values},
        [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("default1", "default2"))));

    load({
        {"another_scope_an_int=something\n", "file1.ini"},
        {"another_scope_an_int=other\n", "file2.ini"},
    });
}

TEST_F(IniFileWithOverridesTest, base_value_wins_over_preset_when_override_is_silent)
{
    config_store.add_int_attribute(a_key, "an int", 5, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Optional(42)));

    load({
        {"a_scope_an_int=42\n", "base.ini"},
        {"another_scope_an_int=1\n", "override.ini"},
    });
}
