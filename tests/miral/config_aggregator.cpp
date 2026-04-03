#include <miral/config_aggregator.h>
#include <miral/live_config_ini_file.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <format>

using namespace testing;
using namespace std::string_literals;
namespace mlc = miral::live_config;

struct ConfigAggregatorTest : Test
{
    mlc::IniFile ini_file;
    mlc::ConfigAggregator aggregator{ini_file};

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

    using FilePairs = std::vector<std::pair<std::reference_wrapper<std::istream>, std::filesystem::path>>;

    auto fake_filename() const
    {
        return UnitTest::GetInstance()->current_test_info()->name();
    }

    void load_one(std::istream& stream, std::filesystem::path path = "test.conf")
    {
        FilePairs files{{std::ref(stream), std::move(path)}};
        aggregator.load_all(files);
    }
};

TEST_F(ConfigAggregatorTest, an_integer_value_is_handled)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, another_integer_value_is_handled)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    aggregator.add_int_attribute(another_key, "another scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));
    EXPECT_CALL(*this, int_handler(another_key, std::optional<int>{64}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_preset_integer_value_is_handled)
{
    mlc::Key const my_key{"my_scope", "thirteen"};

    aggregator.add_int_attribute(my_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(my_key, std::optional<int>{13}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_preset_integer_value_handles_value_from_file)
{
    aggregator.add_int_attribute(a_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_bool_value_is_handled)
{
    aggregator.add_bool_attribute(a_true_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });
    aggregator.add_bool_attribute(a_false_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(a_false_key, std::optional<bool>{false}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_bool_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_bool_attribute(a_true_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });
    aggregator.add_bool_attribute(my_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(my_key, std::optional<bool>{false}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_string_value_is_handled)
{
    aggregator.add_string_attribute(a_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });
    aggregator.add_string_attribute(another_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(another_string_key, std::optional<std::string_view const>{"bar baz"}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_string_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_string_attribute(a_string_key, "a scoped string", "ignored", [this](auto... args) { string_handler(args...); });
    aggregator.add_string_attribute(my_key, "a scoped string", "(none)", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(my_key, std::optional<std::string_view const>{"(none)"}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_float_value_is_handled)
{
    aggregator.add_float_attribute(a_real_key, "a float", [this](auto... args) { float_handler(args...); });
    aggregator.add_float_attribute(another_real_key, "another float", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, std::optional<float>{3.5f}));
    EXPECT_CALL(*this, float_handler(another_real_key, std::optional<float>{100.0f}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_float_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_float_attribute(a_real_key, "a float", -7.0f, [this](auto... args) { float_handler(args...); });
    aggregator.add_float_attribute(my_key, "a float", 23.0f, [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, std::optional<float>{3.5f}));
    EXPECT_CALL(*this, float_handler(my_key, std::optional<float>{23.0f}));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_strings_value_is_handled)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });
    aggregator.add_strings_attribute(another_strings_key, "more strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(another_strings_key, Optional(ElementsAre("foo bar", "baz"))));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_strings_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_strings_attribute(a_strings_key, "strings", {{"ignored"s}}, [this](auto... args) { strings_handler(args...); });
    aggregator.add_strings_attribute(my_key, "a scoped string", {{"(none)"s}}, [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(my_key, Optional(ElementsAre("(none)"))));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, an_ints_value_is_handled)
{
    aggregator.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });
    aggregator.add_ints_attribute(another_ints_key, "more ints", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, ints_handler(another_ints_key, Optional(ElementsAre(3, 5))));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, an_ints_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_ints_attribute(an_ints_key, "ints", {{42}}, [this](auto... args) { ints_handler(args...); });
    aggregator.add_ints_attribute(my_key, "more ints", {{42, 64}}, [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, ints_handler(my_key, Optional(ElementsAre(42, 64))));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_floats_value_is_handled)
{
    aggregator.add_floats_attribute(a_floats_key, "floats", [this](auto... args) { floats_handler(args...); });
    aggregator.add_floats_attribute(another_floats_key, "more floats", [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, floats_handler(another_floats_key, Optional(ElementsAre(3, 5))));

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_floats_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    aggregator.add_floats_attribute(a_floats_key, "floats", {{42}}, [this](auto... args) { floats_handler(args...); });
    aggregator.add_floats_attribute(my_key, "more floats", {{42, 64}}, [this](auto... args) { floats_handler(args...); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, floats_handler(my_key, Optional(ElementsAre(42, 64))));

    load_one(istream, fake_filename());
}

// Unlike IniFile, ConfigAggregator always calls all registered handlers after load_all.
// A bad (unparseable) value means no valid value was stored, so the handler receives nullopt.
TEST_F(ConfigAggregatorTest, a_bad_integer_value_results_in_nullopt)
{
    std::istringstream bad_istream{a_key.to_string() + "=42x"};

    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt}));

    load_one(bad_istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_bad_float_value_results_in_nullopt)
{
    std::istringstream bad_istream{a_key.to_string() + "=42x"};

    aggregator.add_float_attribute(a_key, "bad float", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_key, std::optional<float>{std::nullopt}));

    load_one(bad_istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, a_bad_bool_value_results_in_nullopt)
{
    std::istringstream bad_istream{a_key.to_string() + "=42x"};

    aggregator.add_bool_attribute(a_key, "bad bool", [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_key, std::optional<bool>{std::nullopt}));

    load_one(bad_istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, done_handlers_run_last)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });
    aggregator.add_strings_attribute(another_strings_key, "more strings", [this](auto... args) { strings_handler(args...); });

    aggregator.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });
    aggregator.add_ints_attribute(another_ints_key, "more ints", [this](auto... args) { ints_handler(args...); });

    aggregator.add_floats_attribute(a_floats_key, "floats", [this](auto... args) { floats_handler(args...); });
    aggregator.add_floats_attribute(another_floats_key, "more floats", [this](auto... args) { floats_handler(args...); });

    aggregator.add_string_attribute(a_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });
    aggregator.add_string_attribute(another_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });

    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    aggregator.add_int_attribute(another_key, "another scoped int", [this](auto... args) { int_handler(args...); });

    aggregator.add_float_attribute(a_real_key, "a float", [this](auto... args) { float_handler(args...); });
    aggregator.add_float_attribute(another_real_key, "another float", [this](auto... args) { float_handler(args...); });

    aggregator.add_bool_attribute(a_true_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });
    aggregator.add_bool_attribute(a_false_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });

    bool done_called = false;
    aggregator.on_done([&] { done_called = true; });

    EXPECT_CALL(*this, strings_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, ints_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, floats_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, string_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, int_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, float_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });
    EXPECT_CALL(*this, bool_handler(_, _)).Times(AnyNumber()).WillRepeatedly([&] { EXPECT_THAT(done_called, IsFalse()); });

    load_one(istream, fake_filename());

    EXPECT_THAT(done_called, IsTrue());
}

TEST_F(ConfigAggregatorTest, the_last_registration_of_a_key_is_used)
{
    aggregator.add_string_attribute(a_key, "strings", [this](auto... args) { string_handler(args...); });
    aggregator.add_strings_attribute(a_key, "strings", [this](auto... args) { strings_handler(args...); });
    aggregator.add_float_attribute(a_key, "a float", [this](auto... args) { float_handler(args...); });
    aggregator.add_floats_attribute(a_key, "floats", [this](auto... args) { floats_handler(args...); });
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, strings_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, float_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, floats_handler(a_key, _)).Times(0);
    EXPECT_CALL(*this, int_handler(a_key, _)).Times(1);

    aggregator.add_string_attribute(another_key, "strings", [this](auto... args) { string_handler(args...); });
    aggregator.add_strings_attribute(another_key, "strings", [this](auto... args) { strings_handler(args...); });
    aggregator.add_float_attribute(another_key, "a float", [this](auto... args) { float_handler(args...); });
    aggregator.add_floats_attribute(another_key, "floats", [this](auto... args) { floats_handler(args...); });
    aggregator.add_ints_attribute(another_key, "a scoped int", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, string_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, strings_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, float_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, floats_handler(another_key, _)).Times(0);
    EXPECT_CALL(*this, ints_handler(another_key, _)).Times(1);

    load_one(istream, fake_filename());
}

TEST_F(ConfigAggregatorTest, unset_key_yields_nullopt)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream{another_key.to_string() + "=42\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt}));

    load_one(stream, fake_filename());
}

TEST_F(ConfigAggregatorTest, done_handler_is_called_on_load_all)
{
    int done_count = 0;
    aggregator.on_done([&done_count] { done_count++; });

    std::istringstream empty_stream{""};
    load_one(empty_stream, fake_filename());

    EXPECT_THAT(done_count, Eq(1));
}

TEST_F(ConfigAggregatorTest, empty_value_clears_earlier_array_entries_in_same_stream)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream{
        a_strings_key.to_string() + "=foo\n" +
        a_strings_key.to_string() + "=bar\n" +
        a_strings_key.to_string() + "=\n" +
        a_strings_key.to_string() + "=baz\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("baz"))));

    load_one(stream, fake_filename());
}

TEST_F(ConfigAggregatorTest, later_file_overrides_earlier_scalar)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{a_key.to_string() + "=10\n"};
    std::istringstream stream2{a_key.to_string() + "=99\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{99}));

    FilePairs files{
        {std::ref(stream1), "base.conf"},
        {std::ref(stream2), "base.conf.d/99-override.conf"}};
    aggregator.load_all(files);
}

TEST_F(ConfigAggregatorTest, array_values_are_appended_across_files)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream1{a_strings_key.to_string() + "=foo\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream3{a_strings_key.to_string() + "=baz\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar", "baz"))));

    FilePairs files{
        {std::ref(stream1), "base.conf"},
        {std::ref(stream2), "base.conf.d/10-override.conf"},
        {std::ref(stream3), "base.conf.d/20-override.conf"}};
    aggregator.load_all(files);
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

    FilePairs files{
        {std::ref(stream1), "base.conf"},
        {std::ref(stream2), "base.conf.d/10-default.conf"},
        {std::ref(stream3), fake_filename()}};
    aggregator.load_all(files);
}

TEST_F(ConfigAggregatorTest, array_clears_override_defaults)
{
    aggregator.add_strings_attribute(
        a_strings_key, "strings", {{"default1"s, "default2"s}}, [this](auto... args) { strings_handler(args...); });

    // stream1 sets array values; stream2 (higher priority) clears them with an empty assignment
    std::istringstream stream1{a_strings_key.to_string() + "=foo\n" + a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(IsEmpty())));

    FilePairs files{
        {std::ref(stream1), "base.conf"},
        {std::ref(stream2), "base.conf.d/10-override.conf"}};
    aggregator.load_all(files);
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
    FilePairs files{{std::ref(stream), "base.conf"}};
    aggregator.load_all(files);

    EXPECT_THAT(done_called, IsTrue());
}

TEST_F(ConfigAggregatorTest, empty_value_in_later_file_clears_entries_from_earlier_files)
{
    aggregator.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });

    std::istringstream stream1{a_strings_key.to_string() + "=foo\n" + a_strings_key.to_string() + "=bar\n"};
    std::istringstream stream2{a_strings_key.to_string() + "=\n" + a_strings_key.to_string() + "=baz\n"};

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("baz"))));

    FilePairs files{
        {std::ref(stream1), "base.conf"},
        {std::ref(stream2), "base.conf.d/10-override.conf"}};
    aggregator.load_all(files);
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
        FilePairs files{{std::ref(stream_objects[i]), filepath}};
        aggregator.load_all(files);
    }
}

TEST_F(ConfigAggregatorTest, done_handler_is_called_once_per_load_all)
{
    int done_count = 0;
    aggregator.on_done([&done_count] { done_count++; });

    std::istringstream stream1{a_key.to_string() + "=1\n"};
    std::istringstream stream2{a_key.to_string() + "=2\n"};
    std::istringstream stream3{a_key.to_string() + "=3\n"};

    FilePairs files1{{std::ref(stream1), "base.conf"}};
    aggregator.load_all(files1);

    FilePairs files2{{std::ref(stream2), "base.conf.d/10-override.conf"}};
    aggregator.load_all(files2);

    FilePairs files3{{std::ref(stream3), "base.conf.d/20-override.conf"}};
    aggregator.load_all(files3);

    EXPECT_THAT(done_count, Eq(3));
}

TEST_F(ConfigAggregatorTest, unassigned_key_yields_nullopt_across_calls)
{
    aggregator.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{another_key.to_string() + "=42\n"};
    std::istringstream stream2{another_key.to_string() + "=64\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{std::nullopt})).Times(2);

    FilePairs files1{{std::ref(stream1), "base.conf"}};
    aggregator.load_all(files1);

    FilePairs files2{{std::ref(stream2), "base.conf.d/10-override.conf"}};
    aggregator.load_all(files2);
}

TEST_F(ConfigAggregatorTest, preset_is_used_when_no_file_assigns_to_key)
{
    aggregator.add_int_attribute(a_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    std::istringstream stream1{another_key.to_string() + "=42\n"};

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{13}));

    FilePairs files{{std::ref(stream1), "base.conf"}};
    aggregator.load_all(files);
}

TEST_F(ConfigAggregatorTest, reload_updates_scalar_value)
{
    mlc::Key const cursor_scale{"cursor_scale"};
    aggregator.add_float_attribute(cursor_scale, "cursor scale", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(_, _)).Times(AnyNumber());
    std::istringstream initial{cursor_scale.to_string() + "=1.0\n"};
    FilePairs files1{{std::ref(initial), "base.conf"}};
    aggregator.load_all(files1);

    EXPECT_CALL(*this, float_handler(cursor_scale, std::optional<float>{2.0f}));
    std::istringstream updated{cursor_scale.to_string() + "=2.0\n"};
    FilePairs files2{{std::ref(updated), "base.conf"}};
    aggregator.load_all(files2);
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
    FilePairs files1{{std::ref(base), "base.conf"}, {std::ref(override_stream), "base.conf.d/99-override.conf"}};
    aggregator.load_all(files1);

    // Base updated to include a_third_key; override still has cursor_scale=2.0 and wins
    std::istringstream updated_base{cursor_scale.to_string() + "=1.0\n" + a_third_key.to_string() + "=hello\n"};
    std::istringstream override_again{cursor_scale.to_string() + "=2.0\n"};

    EXPECT_CALL(*this, float_handler(cursor_scale, std::optional<float>{2.0f}));
    EXPECT_CALL(*this, string_handler(a_third_key, std::optional<std::string_view const>{"hello"}));

    FilePairs files2{
        {std::ref(updated_base), "base.conf"},
        {std::ref(override_again), "base.conf.d/99-override.conf"}};
    aggregator.load_all(files2);
}
