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

#include <miral/config_aggregator.h>
#include <miral/live_config_ini_file.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <format>
#include <memory>

using namespace testing;
using namespace std::string_literals;
namespace mlc = miral::live_config;

struct ConfigAggregatorTest : Test
{
    mlc::ConfigAggregator aggregator{};

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, float_handler, (mlc::Key const& key, std::optional<float> value));
    MOCK_METHOD(void, string_handler, (mlc::Key const& key, std::optional<std::string_view const> value));
    MOCK_METHOD(void, strings_handler, (mlc::Key const& key, std::optional<std::span<std::string const>> value));
    MOCK_METHOD(void, ints_handler, (mlc::Key const& key, std::optional<std::span<int const>> value));

    mlc::Key const a_key{"a_scope", "an_int"};
    mlc::Key const another_key{"another_scope", "an_int"};
    mlc::Key const a_string_key{"a_string"};
    mlc::Key const a_strings_key{"strings"};
    mlc::Key const an_ints_key{"ints"};

    using FilePairs = std::vector<std::pair<std::reference_wrapper<std::istream>, std::filesystem::path>>;

    void load_all(FilePairs streams_paths)
    {
        for (auto const& [stream, path] : streams_paths)
        {
            auto ini_file = std::make_shared<mlc::IniFile>();
            aggregator.add_source({
                ini_file,
                [=] { ini_file->load_file(stream, path); },
                path,
            });
        }

        aggregator.reload_all();

        // Cleanup
        for (auto const& [_, path] : streams_paths)
            aggregator.remove_source(path);
    }
};

TEST_F(ConfigAggregatorTest, later_file_overrides_earlier_scalar)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{a_key.to_string() + "=10\n"};
    std::istringstream stream2{a_key.to_string() + "=99\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{99}));

    load_all({{std::ref(stream1), "base.conf"}, {std::ref(stream2), "base.conf.d/99-override.conf"}});
}

TEST_F(ConfigAggregatorTest, array_values_are_appended_across_files)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream1{a_strings_key.to_string() + "=foo\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream3{a_strings_key.to_string() + "=baz\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar", "baz"))));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"},
              {std::ref(stream3), "base.conf.d/20-override.conf"}});
}

TEST_F(ConfigAggregatorTest, combined_state_aggregated_across_files)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream1{};
    std::istringstream stream2{
        a_key.to_string() + "=42\n" +
        a_strings_key.to_string() + "=foo\n" +
        a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream3{
        a_key.to_string() + "=43\n" +
        a_strings_key.to_string() + "=baz\n" +
        a_strings_key.to_string() + "=qux\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{43}));
    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar", "baz", "qux"))));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-default.conf"},
              {std::ref(stream3), "base.conf.d/20-override.conf"}});
}

TEST_F(ConfigAggregatorTest, array_clears_override_defaults)
{
    aggregator.add_strings_attribute(
        a_strings_key, "strings", {{"default1"s, "default2"s}}, [this](auto... args) { strings_handler(args...); });

    // stream1 sets array values; stream2 (higher priority) clears them with an empty assignment
    std::istringstream stream1{a_strings_key.to_string() + "=foo\n" + a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Eq(std::nullopt)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, done_handlers_run_last_with_load_all)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    bool done_called = false;
    aggregator.on_done([&] { done_called = true; });

    EXPECT_CALL(*this, int_handler(_, _)).WillOnce([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, strings_handler(_, _)).WillOnce([&] { EXPECT_THAT(done_called, IsFalse()); });

    std::istringstream stream{a_key.to_string() + "=42\n" + a_strings_key.to_string() + "=foo\n"};
    load_all({{std::ref(stream), "base.conf"}});

    EXPECT_THAT(done_called, IsTrue());
}

TEST_F(ConfigAggregatorTest, empty_value_in_later_file_clears_entries_from_earlier_files)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream1{a_strings_key.to_string() + "=foo\n" + a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=\n" + a_strings_key.to_string() + "=baz\n"};

    InSequence seq;
    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("baz"))));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, handlers_are_called_exactly_once_per_load_all)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    aggregator.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    auto constexpr num_loads = 7;
    std::istringstream stream_objects[num_loads];

    EXPECT_CALL(*this, int_handler(a_key, _)).Times(num_loads);
    EXPECT_CALL(*this, ints_handler(an_ints_key, _)).Times(num_loads);

    for (auto i = 0; i < num_loads; ++i)
    {
        stream_objects[i] = std::istringstream{
            a_key.to_string() + "=10\n" + an_ints_key.to_string() + "=20\n" + an_ints_key.to_string() + "=30\n"};
        auto const filepath = std::filesystem::path("base.conf.d/") / std::format("{:02}-override.conf", i);
        load_all({{std::ref(stream_objects[i]), filepath}});
    }
}

TEST_F(ConfigAggregatorTest, done_handler_is_called_once_per_load_all)
{
    int done_count = 0;
    aggregator.on_done([&done_count] { done_count++; });

    std::istringstream stream1{a_key.to_string() + "=1\n"};
    std::istringstream stream2{a_key.to_string() + "=2\n"};
    std::istringstream stream3{a_key.to_string() + "=3\n"};

    load_all({{std::ref(stream1), "base.conf"}});
    load_all({{std::ref(stream2), "base.conf.d/10-override.conf"}});
    load_all({{std::ref(stream3), "base.conf.d/20-override.conf"}});

    EXPECT_THAT(done_count, Eq(3));
}

TEST_F(ConfigAggregatorTest, unassigned_key_yields_nullopt_across_calls)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{another_key.to_string() + "=42\n"};
    std::istringstream stream2{another_key.to_string() + "=64\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt})).Times(2);

    load_all({{std::ref(stream1), "base.conf"}});
    load_all({{std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, reload_updates_scalar_value)
{
    mlc::Key const cursor_scale{"cursor_scale"};
    aggregator.add_float_attribute(cursor_scale, "cursor scale", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(_, _)).Times(AnyNumber());
    std::istringstream initial{cursor_scale.to_string() + "=1.0\n"};
    load_all({{std::ref(initial), "base.conf"}});

    EXPECT_CALL(*this, float_handler(cursor_scale, std::optional<float>{2.0f}));
    std::istringstream updated{cursor_scale.to_string() + "=2.0\n"};
    load_all({{std::ref(updated), "base.conf"}});
}

TEST_F(ConfigAggregatorTest, override_wins_for_shared_key_after_base_update)
{
    mlc::Key const cursor_scale{"cursor_scale"};
    mlc::Key const a_third_key{"a_third_key"};
    aggregator.add_float_attribute(cursor_scale, "cursor scale", [this](auto... args) { float_handler(args...); });
    aggregator.add_string_attribute(a_third_key, "a third key", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, float_handler(_, _)).Times(AnyNumber());
    EXPECT_CALL(*this, string_handler(_, _)).Times(AnyNumber());
    std::istringstream base{cursor_scale.to_string() + "=1.0\n"};
    std::istringstream override_stream{cursor_scale.to_string() + "=2.0\n"};
    load_all({{std::ref(base), "base.conf"}, {std::ref(override_stream), "base.conf.d/99-override.conf"}});

    // Base updated to include a_third_key; override still has cursor_scale=2.0 and wins
    std::istringstream updated_base{cursor_scale.to_string() + "=1.0\n" + a_third_key.to_string() + "=hello\n"};
    std::istringstream override_again{cursor_scale.to_string() + "=2.0\n"};

    EXPECT_CALL(*this, float_handler(cursor_scale, std::optional<float>{2.0f}));
    EXPECT_CALL(*this, string_handler(a_third_key, std::optional<std::string_view const>{"hello"}));

    load_all({{std::ref(updated_base), "base.conf"},
              {std::ref(override_again), "base.conf.d/99-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_scalar_value_in_one_of_multiple_yields_nullopt_valid_value_across_streams_without_appending)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{a_key.to_string() + "=10\n"};
    std::istringstream stream2{a_key.to_string() + "=not_a_number\n"};

    EXPECT_CALL(*this, int_handler(a_key, Eq(std::nullopt)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_scalar_value_in_one_of_multiple_yields_nullopt_across_streams_with_appending)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{a_key.to_string() + "=10\n" + a_key.to_string() + "=20\n"};
    std::istringstream stream2{a_key.to_string() + "=30\n" + a_key.to_string() + "=not_a_number\n"};

    EXPECT_CALL(*this, int_handler(a_key, Eq(std::nullopt)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_scalar_value_followed_by_valid_value_in_later_file_yields_valid)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{a_key.to_string() + "=10\n" + a_key.to_string() + "=20\n"};
    std::istringstream stream2{a_key.to_string() + "=30\n" + a_key.to_string() + "=also_not_a_number\n" + a_key.to_string() + "=40\n"};

    EXPECT_CALL(*this, int_handler(a_key, Optional(40)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_array_value_in_one_of_multiple_files_is_ignored)
{
    aggregator.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    std::istringstream stream1{an_ints_key.to_string() + "=1\n" + an_ints_key.to_string() + "=2\n"};
    std::istringstream stream2{an_ints_key.to_string() + "=3\n" + an_ints_key.to_string() + "=not_a_number\n"};

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2, 3))));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_ini_file_among_multiple_files_leaves_scalar_as_nullopt)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    // stream1 is a valid ini file; stream2 is JSON (not valid ini)
    std::istringstream stream1{a_key.to_string() + "=42\n"};
    std::istringstream stream2{R"({"key": "value"})"};

    // JSON content cannot be parsed as ini, so only stream1's value is available.
    // Because stream2 is the higher-priority source but provides nothing parseable for a_key,
    // the scalar from stream1 should be used (no override occurred).
    EXPECT_CALL(*this, int_handler(a_key, Optional(42)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, invalid_ini_file_among_multiple_files_leaves_array_values_from_valid_files)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    // stream1 is a valid ini file; stream2 is JSON (not valid ini)
    std::istringstream stream1{a_strings_key.to_string() + "=foo\n" + a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream2{R"({"strings": ["baz"]})"};

    // JSON content cannot be parsed as ini; array entries from stream1 remain intact
    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}

TEST_F(ConfigAggregatorTest, clear_followed_by_invalid_array_value_yields_empty)
{
    aggregator.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });

    // stream1 provides initial values; stream2 first clears the array (empty assignment)
    // then tries to append an invalid value
    std::istringstream stream1{an_ints_key.to_string() + "=1\n" + an_ints_key.to_string() + "=2\n"};
    std::istringstream stream2{
        an_ints_key.to_string() + "=\n" +       // clears entries from stream1
        an_ints_key.to_string() + "=bad_val\n"  // invalid integer after the clear
    };

    EXPECT_CALL(*this, ints_handler(an_ints_key, Eq(std::nullopt)));

    load_all({{std::ref(stream1), "base.conf"},
              {std::ref(stream2), "base.conf.d/10-override.conf"}});
}
