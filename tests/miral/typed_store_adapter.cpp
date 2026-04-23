/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "basic_store.h"
#include "typed_store_adapter.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace mlc = miral::live_config;

namespace
{
auto const a_path = std::filesystem::path{"/some/config/file"};

struct TypedStoreAdapterTest : Test
{
    mlc::BasicStore store;
    mlc::TypedStoreAdapter adapter{store};

    MOCK_METHOD(void, int_handler, (mlc::Key const&, std::optional<int>));
    MOCK_METHOD(void, bool_handler, (mlc::Key const&, std::optional<bool>));
    MOCK_METHOD(void, float_handler, (mlc::Key const&, std::optional<float>));
    MOCK_METHOD(void, string_handler, (mlc::Key const&, std::optional<std::string_view>));
    MOCK_METHOD(void, strings_handler, (mlc::Key const&, std::optional<std::span<std::string const>>));
    MOCK_METHOD(void, ints_handler, (mlc::Key const&, std::optional<std::span<int const>>));
    MOCK_METHOD(void, floats_handler, (mlc::Key const&, std::optional<std::span<float const>>));

    mlc::Key const an_int_key{"an_int"};
    mlc::Key const a_bool_key{"a_bool"};
    mlc::Key const a_float_key{"a_float"};
    mlc::Key const a_string_key{"a_string"};
    mlc::Key const a_strings_key{"some_strings"};
    mlc::Key const an_ints_key{"some_ints"};
    mlc::Key const a_floats_key{"some_floats"};

    void update_and_run(mlc::Key const& key, std::string const& value)
    {
        store.do_transaction([&]{ store.update_key(key, value, a_path); });
    }

    void run_empty()
    {
        store.do_transaction([]{});
    }
};
}


TEST_F(TypedStoreAdapterTest, valid_int_value_is_handled)
{
    adapter.add_int_attribute(an_int_key, "an integer", 0, [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Optional(42)));

    update_and_run(an_int_key, "42");
}

TEST_F(TypedStoreAdapterTest, valid_bool_true_value_is_handled)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", false, [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Optional(true)));

    update_and_run(a_bool_key, "true");
}

TEST_F(TypedStoreAdapterTest, valid_bool_false_value_is_handled)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", true, [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Optional(false)));

    update_and_run(a_bool_key, "false");
}

TEST_F(TypedStoreAdapterTest, valid_float_value_is_handled)
{
    adapter.add_float_attribute(a_float_key, "a float", 0.0f, [this](auto k, auto v){ float_handler(k, v); });

    EXPECT_CALL(*this, float_handler(a_float_key, Optional(3.5f)));

    update_and_run(a_float_key, "3.5");
}

TEST_F(TypedStoreAdapterTest, valid_string_value_is_handled)
{
    adapter.add_string_attribute(a_string_key, "a string", "default", [this](auto k, auto v){ string_handler(k, v); });

    EXPECT_CALL(*this, string_handler(a_string_key, Optional(std::string_view{"hello"})));

    update_and_run(a_string_key, "hello");
}

TEST_F(TypedStoreAdapterTest, int_preset_is_used_when_no_value_is_set)
{
    adapter.add_int_attribute(an_int_key, "an integer", 99, [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Optional(99)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, bool_preset_is_used_when_no_value_is_set)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", true, [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Optional(true)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, float_preset_is_used_when_no_value_is_set)
{
    adapter.add_float_attribute(a_float_key, "a float", 1.5f, [this](auto k, auto v){ float_handler(k, v); });

    EXPECT_CALL(*this, float_handler(a_float_key, Optional(1.5f)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, string_preset_is_used_when_no_value_is_set)
{
    adapter.add_string_attribute(a_string_key, "a string", "default", [this](auto k, auto v){ string_handler(k, v); });

    EXPECT_CALL(*this, string_handler(a_string_key, Optional(std::string_view{"default"})));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, int_preset_is_used_when_value_cannot_be_parsed)
{
    adapter.add_int_attribute(an_int_key, "an integer", 99, [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Optional(99)));

    update_and_run(an_int_key, "not_a_number");
}

TEST_F(TypedStoreAdapterTest, bool_preset_is_used_when_value_cannot_be_parsed)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", true, [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Optional(true)));

    update_and_run(a_bool_key, "not_a_bool");
}

TEST_F(TypedStoreAdapterTest, float_preset_is_used_when_value_cannot_be_parsed)
{
    adapter.add_float_attribute(a_float_key, "a float", 1.5f, [this](auto k, auto v){ float_handler(k, v); });

    EXPECT_CALL(*this, float_handler(a_float_key, Optional(1.5f)));

    update_and_run(a_float_key, "not_a_float");
}

TEST_F(TypedStoreAdapterTest, int_handler_receives_nullopt_when_no_preset_and_no_value)
{
    adapter.add_int_attribute(an_int_key, "an integer", [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, bool_handler_receives_nullopt_when_no_preset_and_no_value)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, float_handler_receives_nullopt_when_no_preset_and_no_value)
{
    adapter.add_float_attribute(a_float_key, "a float", [this](auto k, auto v){ float_handler(k, v); });

    EXPECT_CALL(*this, float_handler(a_float_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, string_handler_receives_nullopt_when_no_preset_and_no_value)
{
    adapter.add_string_attribute(a_string_key, "a string", [this](auto k, auto v){ string_handler(k, v); });

    EXPECT_CALL(*this, string_handler(a_string_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, int_handler_receives_nullopt_when_no_preset_and_bad_value)
{
    adapter.add_int_attribute(an_int_key, "an integer", [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Eq(std::nullopt)));

    update_and_run(an_int_key, "not_a_number");
}

TEST_F(TypedStoreAdapterTest, bool_handler_receives_nullopt_when_no_preset_and_bad_value)
{
    adapter.add_bool_attribute(a_bool_key, "a bool", [this](auto k, auto v){ bool_handler(k, v); });

    EXPECT_CALL(*this, bool_handler(a_bool_key, Eq(std::nullopt)));

    update_and_run(a_bool_key, "not_a_bool");
}

TEST_F(TypedStoreAdapterTest, float_handler_receives_nullopt_when_no_preset_and_bad_value)
{
    adapter.add_float_attribute(a_float_key, "a float", [this](auto k, auto v){ float_handler(k, v); });

    EXPECT_CALL(*this, float_handler(a_float_key, Eq(std::nullopt)));

    update_and_run(a_float_key, "not_a_float");
}

TEST_F(TypedStoreAdapterTest, valid_ints_values_are_handled)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2, 3))));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "1", a_path);
        store.update_key(an_ints_key, "2", a_path);
        store.update_key(an_ints_key, "3", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, valid_floats_values_are_handled)
{
    adapter.add_floats_attribute(a_floats_key, "some floats", [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(1.0f, 2.5f))));

    store.do_transaction([&]
    {
        store.update_key(a_floats_key, "1", a_path);
        store.update_key(a_floats_key, "2.5", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, valid_strings_values_are_handled)
{
    adapter.add_strings_attribute(a_strings_key, "some strings", [this](auto k, auto v){ strings_handler(k, v); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));

    store.do_transaction([&]
    {
        store.update_key(a_strings_key, "foo", a_path);
        store.update_key(a_strings_key, "bar", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, ints_preset_is_used_when_no_values_are_set)
{
    std::vector<int> const preset_vals{10, 20};
    adapter.add_ints_attribute(an_ints_key, "some ints", preset_vals, [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAreArray(preset_vals))));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, ints_preset_is_used_when_all_values_cannot_be_parsed)
{
    std::vector<int> const preset_vals{10, 20};
    adapter.add_ints_attribute(an_ints_key, "some ints", preset_vals, [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAreArray(preset_vals))));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "not_a_number", a_path);
        store.update_key(an_ints_key, "also_not_a_number", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, floats_preset_is_used_when_no_values_are_set)
{
    std::vector<float> const preset_vals{1.1f, 2.2f};
    adapter.add_floats_attribute(a_floats_key, "some floats", preset_vals, [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAreArray(preset_vals))));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, floats_preset_is_used_when_all_values_cannot_be_parsed)
{
    std::vector<float> const preset_vals{1.1f, 2.2f};
    adapter.add_floats_attribute(a_floats_key, "some floats", preset_vals, [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAreArray(preset_vals))));

    store.do_transaction([&]
    {
        store.update_key(a_floats_key, "not_a_number", a_path);
        store.update_key(a_floats_key, "also_not_a_number", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, strings_preset_is_used_when_no_values_are_set)
{
    std::vector<std::string> const preset_vals{"alpha", "beta"};
    adapter.add_strings_attribute(a_strings_key, "some strings", preset_vals, [this](auto k, auto v){ strings_handler(k, v); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAreArray(preset_vals))));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, ints_bad_element_is_skipped_and_good_values_are_handled)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1))));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "1", a_path);
        store.update_key(an_ints_key, "not_a_number", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, floats_bad_element_is_skipped_and_good_values_are_handled)
{
    adapter.add_floats_attribute(a_floats_key, "some floats", [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Optional(ElementsAre(FloatEq(1.0f)))));

    store.do_transaction([&]
    {
        store.update_key(a_floats_key, "1.0", a_path);
        store.update_key(a_floats_key, "not_a_float", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, ints_handler_receives_nullopt_when_no_preset_and_no_values)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, floats_handler_receives_nullopt_when_no_preset_and_no_values)
{
    adapter.add_floats_attribute(a_floats_key, "some floats", [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, strings_handler_receives_nullopt_when_no_preset_and_no_values)
{
    adapter.add_strings_attribute(a_strings_key, "some strings", [this](auto k, auto v){ strings_handler(k, v); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Eq(std::nullopt)));

    run_empty();
}

TEST_F(TypedStoreAdapterTest, ints_handler_receives_nullopt_when_no_preset_and_all_values_are_bad)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Eq(std::nullopt)));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "not_a_number", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, floats_handler_receives_nullopt_when_no_preset_and_all_values_are_bad)
{
    adapter.add_floats_attribute(a_floats_key, "some floats", [this](auto k, auto v){ floats_handler(k, v); });

    EXPECT_CALL(*this, floats_handler(a_floats_key, Eq(std::nullopt)));

    store.do_transaction([&]
    {
        store.update_key(a_floats_key, "not_a_float", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, clear_array_within_transaction_removes_previously_accumulated_values)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    InSequence seq;
    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(IsEmpty())));
    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(3))));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "1", a_path);
        store.update_key(an_ints_key, "2", a_path);
        store.clear_array(an_ints_key);
        store.update_key(an_ints_key, "3", a_path);
    });
}

TEST_F(TypedStoreAdapterTest, clear_array_within_transaction_with_no_following_values_results_in_empty)
{
    adapter.add_ints_attribute(an_ints_key, "some ints", [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(IsEmpty())));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "1", a_path);
        store.update_key(an_ints_key, "2", a_path);
        store.clear_array(an_ints_key);
    });
}

TEST_F(TypedStoreAdapterTest, clear_array_within_transaction_with_preset_and_no_following_values_clears_the_array)
{
    std::vector<int> const preset_vals{10, 20};
    adapter.add_ints_attribute(an_ints_key, "some ints", preset_vals, [this](auto k, auto v){ ints_handler(k, v); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(IsEmpty())));

    store.do_transaction([&]
    {
        store.update_key(an_ints_key, "1", a_path);
        store.clear_array(an_ints_key);
    });
}

TEST_F(TypedStoreAdapterTest, clear_array_on_unregistered_key_does_not_crash)
{
    mlc::Key const unregistered_key{"unregistered_key"};

    EXPECT_NO_THROW(store.do_transaction([&]
    {
        store.clear_array(unregistered_key);
    }));
}

TEST_F(TypedStoreAdapterTest, clear_array_on_scalar_key_does_not_affect_its_value)
{
    adapter.add_int_attribute(an_int_key, "an integer", [this](auto k, auto v){ int_handler(k, v); });

    EXPECT_CALL(*this, int_handler(an_int_key, Optional(42)));

    store.do_transaction([&]
    {
        store.update_key(an_int_key, "42", a_path);
        store.clear_array(an_int_key);
    });
}
