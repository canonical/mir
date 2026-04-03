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

#ifndef MIRAL_LIVE_CONFIG_BASIC_STORE_H
#define MIRAL_LIVE_CONFIG_BASIC_STORE_H

#include <miral/live_config.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>

namespace miral
{
namespace live_config
{
class BasicStore : public Store
{
public:
    using TypedScalar = std::variant<int, bool, float, std::string>;
    using TypedArray = std::variant<std::vector<int>, std::vector<bool>, std::vector<float>, std::vector<std::string>>;

    BasicStore();

    void add_int_attribute(Key const& key, std::string_view description, HandleInt handler) override;
    void add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, HandleFloat handler) override;
    void add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler) override;
    void add_string_attribute(Key const& key, std::string_view description, HandleString handler) override;
    void add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) override;

    void add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler) override;
    void add_ints_attribute(
        Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler) override;
    void add_floats_attribute(
        Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler) override;
    void add_string_attribute(
        Key const& key, std::string_view description, std::string_view preset, HandleString handler) override;
    void add_strings_attribute(
        Key const& key,
        std::string_view description,
        std::span<std::string const> preset,
        HandleStrings handler) override;

    void on_done(HandleDone handler) override;

    bool update_key(Key const& key, std::string_view value, std::filesystem::path modification_path);

    /// Update a scalar value from an already-typed value (used by ConfigAggregator).
    /// No string conversion is performed.
    template <typename T> void update_typed_value(Key const& key, T value, std::filesystem::path const& path)
    {
        static_assert(
            std::is_same_v<T, int> || std::is_same_v<T, bool> || std::is_same_v<T, float>
                || std::is_same_v<T, std::string_view>,
            "update_typed_value only supports int, bool, float, or std::string_view");

        if constexpr (std::is_same_v<T, std::string_view>)
            update_typed_value_impl(key, TypedScalar{std::string{value}}, path);
        else
            update_typed_value_impl(key, TypedScalar{value}, path);
    }

    /// Append an array element from an already-typed value (used by ConfigAggregator).
    /// No string conversion is performed.
    template <typename T> void update_typed_array_value(Key const& key, T value, std::filesystem::path const& path)
    {
        static_assert(
            std::is_same_v<T, int> || std::is_same_v<T, bool> || std::is_same_v<T, float>
                || std::is_same_v<T, std::string>,
            "update_typed_array_value only supports int, bool, float, or std::string");

        if constexpr (std::is_same_v<T, std::string>)
            update_typed_array_value_impl(key, TypedArray{std::vector<std::string>{std::string{value}}}, path);
        else
            update_typed_array_value_impl(key, TypedArray{std::vector<T>{std::move(value)}}, path);
    }

    void clear_array(Key const& key);

    void clear_values();
    void call_attribute_handlers() const;
    void call_done_handlers(std::string const& paths) const;

private:
    void update_typed_value_impl(Key const& key, TypedScalar value, std::filesystem::path const& path);
    void update_typed_array_value_impl(Key const& key, TypedArray value, std::filesystem::path const& path);

    struct Self;
    std::shared_ptr<Self> self;
};
}
}

#endif
