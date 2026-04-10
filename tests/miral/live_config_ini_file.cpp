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

#include <miral/live_config_ini_file.h>


#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace std::string_literals;
namespace mlc = miral::live_config;

struct LiveConfigIniFile : Test
{
    mlc::IniFile ini_file;

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, bool_handler, (mlc::Key const& key, std::optional<bool> value));
    MOCK_METHOD(void, float_handler, (mlc::Key const& key, std::optional<float> value));
    MOCK_METHOD(void, string_handler, (mlc::Key const& key, std::optional<std::string_view const> value));
    MOCK_METHOD(void, strings_handler, (mlc::Key const& key, std::optional<std::span<std::string const>> value));
    MOCK_METHOD(void, ints_handler, (mlc::Key const& key, std::optional<std::span<int const>> value));
    MOCK_METHOD(void, floats_handler, (mlc::Key const& key, std::optional<std::span<float const>> value));

    mlc::Key const a_key{"a_scope", "an_int"};
    mlc::Key const another_key{"another_scope", "an_int"};
    mlc::Key const a_true_key{"a_scope", "a_bool"};
    mlc::Key const a_false_key{"a_scope", "another_bool"};
    mlc::Key const a_string_key{"a_string"};
    mlc::Key const another_string_key{"another_string"};
    mlc::Key const a_real_key{"a_float"};
    mlc::Key const another_real_key{"another_float"};
    mlc::Key const a_strings_key{"strings"};
    mlc::Key const another_strings_key{"more_strings"};
    mlc::Key const an_ints_key{"ints"};
    mlc::Key const another_ints_key{"more_ints"};
    mlc::Key const a_floats_key{"floats"};
    mlc::Key const another_floats_key{"more_floats"};

    std::istringstream istream{
        a_key.to_string()+"=42\n" +
        another_key.to_string()+"=64\n" +
        a_true_key.to_string()+"=true\n" +
        a_false_key.to_string()+"=false\n" +
        a_string_key.to_string()+"=foo\n" +
        another_string_key.to_string()+"=bar baz\n" +
        a_real_key.to_string()+"=3.5\n" +
        another_real_key.to_string()+"=1e2\n" +
        a_strings_key.to_string()+"=foo\n" +
        a_strings_key.to_string()+"=bar\n" +
        another_strings_key.to_string()+"=foo bar\n" +
        another_strings_key.to_string()+"=baz\n" +
        an_ints_key.to_string()+"=1\n" +
        an_ints_key.to_string()+"=2\n" +
        another_ints_key.to_string()+"=3\n" +
        another_ints_key.to_string()+"=5\n" +
        a_floats_key.to_string()+"=1\n" +
        a_floats_key.to_string()+"=2\n" +
        another_floats_key.to_string()+"=3\n" +
        another_floats_key.to_string()+"=5\n"
    };

    auto fake_filename() const
    {
        return UnitTest::GetInstance()->current_test_info()->name();
    }
};

TEST_F(LiveConfigIniFile, an_integer_value_is_handled)
{
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, another_integer_value_is_handled)
{
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_int_attribute(another_key, "another scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));
    EXPECT_CALL(*this, int_handler(another_key, std::optional<int>{64}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_preset_integer_value_is_handled)
{
    mlc::Key const my_key{"my_scope", "thirteen"};

    ini_file.add_int_attribute(my_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(my_key, std::optional<int>{13}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_preset_integer_value_handles_value_from_file)
{
    ini_file.add_int_attribute(a_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bool_value_is_handled)
{
    ini_file.add_bool_attribute(a_true_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });
    ini_file.add_bool_attribute(a_false_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(a_false_key, std::optional<bool>{false}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bool_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_bool_attribute(a_true_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });
    ini_file.add_bool_attribute(my_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(my_key, std::optional<bool>{false}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_string_value_is_handled)
{
    ini_file.add_string_attribute(a_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(another_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(another_string_key, std::optional<std::string_view const>{"bar baz"}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_string_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_string_attribute(a_string_key, "a scoped string", "ignored", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(my_key, "a scoped string", "(none)", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(my_key, std::optional<std::string_view const>{"(none)"}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_float_value_is_handled)
{
    ini_file.add_float_attribute(a_real_key, "a float", [this](auto... args) { float_handler(args...); });
    ini_file.add_float_attribute(another_real_key, "another float", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, {3.5}));
    EXPECT_CALL(*this, float_handler(another_real_key, {100.0}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_float_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_float_attribute(a_real_key, "a float", -7.0, [this](auto... args) { float_handler(args...); });
    ini_file.add_float_attribute(my_key, "a scoped bool", 23.0, [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, {3.5}));
    EXPECT_CALL(*this, float_handler(my_key, {23}));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_strings_value_is_handled)
{
    ini_file.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });
    ini_file.add_strings_attribute(another_strings_key, "more strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(another_strings_key, Optional(ElementsAre("foo bar", "baz"))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_strings_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_strings_attribute(a_strings_key, "strings", {{"ignored"s}}, [this](auto... args) { strings_handler(args...); });
    ini_file.add_strings_attribute(my_key, "a scoped string", {{"(none)"s}}, [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(my_key, Optional(ElementsAre("(none)"))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_ints_value_is_handled)
{
    ini_file.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(another_ints_key, "more ints", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, ints_handler(another_ints_key, Optional(ElementsAre(3, 5))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_ints_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_ints_attribute(an_ints_key, "ints", {{42}}, [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(my_key, "more ints", {{42, 64}}, [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, ints_handler(my_key, Optional(ElementsAre(42, 64))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_floats_value_is_handled)
{
    ini_file.add_floats_attribute(a_floats_key, "floats", [this](auto... args) { floats_handler(args...); });
    ini_file.add_floats_attribute(another_floats_key, "more floats", [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, floats_handler(another_floats_key, Optional(ElementsAre(3, 5))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_floats_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_floats_attribute(a_floats_key, "floats", {{42}}, [this](auto... args) { floats_handler(args...); });
    ini_file.add_floats_attribute(my_key, "more floats", {{42, 64}}, [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, floats_handler(my_key, Optional(ElementsAre(42, 64))));

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bad_integer_value_is_ignored)
{
    std::istringstream bad_istream{
        a_key.to_string()+"=42x"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, _)).Times(0);

    ini_file.load_file(bad_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bad_float_value_is_ignored)
{
    std::istringstream bad_istream{
        a_key.to_string()+"=42x"};

    ini_file.add_float_attribute(a_key, "bad float", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_key, _)).Times(0);

    ini_file.load_file(bad_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bad_bool_value_is_ignored)
{
    std::istringstream bad_istream{
        a_key.to_string()+"=42x"};

    ini_file.add_bool_attribute(a_key, "bad bool", [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_key, _)).Times(0);

    ini_file.load_file(bad_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bad_integer_value_in_an_array_is_skipped)
{
    std::istringstream bad_istream{
        an_ints_key.to_string()+"=1\n" +
        an_ints_key.to_string()+"=42x\n" +
        an_ints_key.to_string()+"=2"};

    ini_file.add_ints_attribute(an_ints_key, "bad int array", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));

    ini_file.load_file(bad_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_bad_float_value_in_an_array_is_skipped)
{
    std::istringstream bad_istream{
        a_floats_key.to_string()+"=1.5\n" +
        a_floats_key.to_string()+"=42x\n" +
        a_floats_key.to_string()+"=2.5"};

    ini_file.add_floats_attribute(a_floats_key, "bad float array", [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1.5f, 2.5f))));

    ini_file.load_file(bad_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, done_handlers_run_last)
{
    ini_file.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });
    ini_file.add_strings_attribute(another_strings_key, "more strings", [this](auto... args) { strings_handler(args...); });

    ini_file.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(another_ints_key, "more ints", [this](auto... args) { ints_handler(args...); });

    ini_file.add_floats_attribute(a_floats_key, "floats", [this](auto... args) { floats_handler(args...); });
    ini_file.add_floats_attribute(another_floats_key, "more floats", [this](auto... args) { floats_handler(args...); });

    ini_file.add_string_attribute(a_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(another_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_int_attribute(another_key, "another scoped int", [this](auto... args) { int_handler(args...); });

    ini_file.add_float_attribute(a_real_key, "a float", [this](auto... args) { float_handler(args...); });
    ini_file.add_float_attribute(another_real_key, "another float", [this](auto... args) { float_handler(args...); });

    ini_file.add_bool_attribute(a_true_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });
    ini_file.add_bool_attribute(a_false_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });

    bool done_called = false;

    ini_file.on_done([&] { done_called = true;});

    EXPECT_CALL(*this, strings_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, ints_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, floats_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, string_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, int_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, float_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, bool_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&]{ EXPECT_THAT(done_called, IsFalse()); });

    ini_file.load_file(istream, fake_filename());

    EXPECT_THAT(done_called, IsTrue());
}

TEST_F(LiveConfigIniFile, the_last_registration_of_a_key_is_used)
{
    ini_file.add_string_attribute(a_key, "strings", [this](auto... args) { string_handler(args...); });
    ini_file.add_strings_attribute(a_key, "strings", [this](auto... args) { strings_handler(args...); });
    ini_file.add_float_attribute(a_key, "a float", [this](auto... args) { float_handler(args...); });
    ini_file.add_floats_attribute(a_key, "floats", [this](auto... args) { floats_handler(args...); });
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, strings_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, float_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, floats_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, int_handler(a_key, _)).Times(1);

    ini_file.add_string_attribute(another_key, "strings", [this](auto... args) { string_handler(args...); });
    ini_file.add_strings_attribute(another_key, "strings", [this](auto... args) { strings_handler(args...); });
    ini_file.add_float_attribute(another_key, "a float", [this](auto... args) { float_handler(args...); });
    ini_file.add_floats_attribute(another_key, "floats", [this](auto... args) { floats_handler(args...); });
    ini_file.add_ints_attribute(another_key, "a scoped int", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, string_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, strings_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, float_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, floats_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, ints_handler(another_key, _)).Times(1);

    ini_file.load_file(istream, fake_filename());
}

TEST_F(LiveConfigIniFile, unparsable_values_in_scalars_are_skipped)
{
    std::istringstream unparsable_value_istream{R"(
        valid_int=42
        unparsable_int=a_string
        unparsable_int_with_preset=another_string
        valid_float=3.5
    )"};

    auto const parsable_int = mlc::Key{"valid_int"};
    auto const unparsable_int = mlc::Key{"unparsable_int"};
    auto const unparsable_int_with_preset = mlc::Key{"unparsable_int_with_preset"};
    auto const parsable_float = mlc::Key{"valid_float"};

    ini_file.add_int_attribute(parsable_int, "a valid int key", [this](auto... args) { int_handler(args...); });
    ini_file.add_int_attribute(unparsable_int, "an unparsable int key", [this](auto... args) { int_handler(args...); });
    ini_file.add_int_attribute(
        unparsable_int_with_preset,
        "an unparsable int key with a preset value",
        12,
        [this](auto... args) { int_handler(args...); });
    ini_file.add_float_attribute(parsable_float, "a valid float key", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, int_handler(parsable_int, Eq(42))).Times(1);
    EXPECT_CALL(*this, int_handler(unparsable_int, _)).Times(0);
    EXPECT_CALL(*this, int_handler(unparsable_int_with_preset, _)).Times(0);
    EXPECT_CALL(*this, float_handler(parsable_float, Eq(3.5))).Times(1);

    ini_file.load_file(unparsable_value_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, unparsable_values_in_arrays_are_skipped)
{
    std::istringstream unparsable_value_istream{R"(
        parsable_ints=10
        parsable_ints=20
        unparsable_ints=a_string
        unparsable_ints=b_string
        unparsable_ints_with_preset=another_string
        parsable_floats=3.5
        parsable_floats=4.5
    )"};

    auto const parsable_ints = mlc::Key{"parsable_ints"};
    auto const unparsable_ints = mlc::Key{"unparsable_ints"};
    auto const unparsable_ints_with_preset = mlc::Key{"unparsable_ints_with_preset"};
    auto const parsable_floats = mlc::Key{"parsable_floats"};

    int const preset_ints[] = {11, 12};

    ini_file.add_ints_attribute(parsable_ints, "a valid ints key", [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(unparsable_ints, "an unparsable ints key", [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(
        unparsable_ints_with_preset,
        "an unparsable ints key with a preset value",
        std::span{preset_ints},
        [this](auto... args) { ints_handler(args...); });
    ini_file.add_floats_attribute(parsable_floats, "a valid floats key", [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, ints_handler(parsable_ints, Optional(ElementsAre(10, 20)))).Times(1);
    EXPECT_CALL(*this, floats_handler(parsable_floats, Optional(ElementsAre(3.5f, 4.5f)))).Times(1);

    EXPECT_CALL(*this, ints_handler(unparsable_ints, Optional(ElementsAre()))).Times(1);
    EXPECT_CALL(*this, ints_handler(unparsable_ints_with_preset, Optional(ElementsAre()))).Times(1);

    ini_file.load_file(unparsable_value_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, non_ini_content_is_ignored)
{
    std::istringstream yaml_containing_istream{R"(
        a_scope_an_int=12
        yaml_key: value
        a_string=foo
    )"};

    auto const yaml_key = mlc::Key{"yaml_key"};
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_string_attribute(yaml_key, "a yaml key", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, Optional(12)));
    EXPECT_CALL(*this, string_handler(yaml_key, std::optional<std::string_view const>{}));
    EXPECT_CALL(*this, string_handler(a_string_key, Optional(std::string_view{"foo"})));

    ini_file.load_file(yaml_containing_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_empty_key_is_skipped)
{
    std::istringstream no_key_istream{"=some_value"};

    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{std::nullopt}));

    ini_file.load_file(no_key_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_empty_string_value_is_passed_to_string_handler)
{
    std::istringstream no_value_istream{"a_string="};

    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{""}));

    ini_file.load_file(no_value_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_empty_int_value_is_ignored)
{
    std::istringstream empty_value_istream{R"(
        a_scope_an_int=12
        a_scope_an_int=
    )"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, _)).Times(0);

    ini_file.load_file(empty_value_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_value_containing_equals_is_split_at_the_first_equals)
{
    std::istringstream extra_equal_istream{"a_string=foo=bar"};

    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo=bar"}));

    ini_file.load_file(extra_equal_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, whitespace_is_trimmed_from_around_keys_and_values)
{
    std::istringstream whitespace_istream{" a_string = foo bar "};

    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo bar"}));

    ini_file.load_file(whitespace_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, blank_and_whitespace_lines_are_ignored_and_valid_lines_are_processed)
{
    std::istringstream whitespace_and_blanks_istream{R"(

        a_scope_an_int=42


        a_string=foo
    )"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));
    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));

    ini_file.load_file(whitespace_and_blanks_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, an_empty_file_results_in_nullopt_values)
{
    std::istringstream empty_istream{""};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_string_attribute(a_string_key, "a string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt}));
    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{std::nullopt}));

    ini_file.load_file(empty_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_file_with_only_comments_results_in_nullopt_values)
{
    std::istringstream comment_istream{
        "# this is a comment\n"
        "# another comment\n"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt}));

    ini_file.load_file(comment_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, a_file_with_only_blank_lines_results_in_nullopt_values)
{
    std::istringstream blank_istream{"\n\n\n"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt}));

    ini_file.load_file(blank_istream, fake_filename());
}

TEST_F(LiveConfigIniFile, loading_a_file_twice_does_not_accumulate_array_values)
{
    ini_file.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2)))).Times(2);

    std::istringstream first_load{"ints=1\nints=2"};
    ini_file.load_file(first_load, fake_filename());

    std::istringstream second_load{"ints=1\nints=2"};
    ini_file.load_file(second_load, fake_filename());
}

TEST_F(LiveConfigIniFile, loading_a_file_twice_uses_the_latest_file_values)
{
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    InSequence seq;
    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));
    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{99}));

    std::istringstream first_load{"a_scope_an_int=42"};
    ini_file.load_file(first_load, fake_filename());

    std::istringstream second_load{"a_scope_an_int=99"};
    ini_file.load_file(second_load, fake_filename());
}
