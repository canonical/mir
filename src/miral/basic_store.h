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

#ifndef MIRAL_BASIC_STORE_H
#define MIRAL_BASIC_STORE_H

#include <miral/live_config.h>

#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <variant>

namespace miral::live_config
{
class BasicStore
{
public:
    /// Represents "never assigned, including no preset"
    struct ArrayUnset {};

    /// The array was appended to (no explicit clear in this transaction).
    template<typename T>
    struct ArrayAppended
    {
        std::span<T const> values;
    };

    /// The array was explicitly cleared, then optionally new values were assigned.
    template<typename T>
    struct ArrayReset
    {
        std::span<T const> values;
    };

    /// Tri-state result passed to array attribute handlers registered with BasicStore.
    template<typename T>
    using ArrayUpdate = std::variant<ArrayUnset, ArrayAppended<T>, ArrayReset<T>>;

    using HandleInts    = std::function<void(Key const& key, ArrayUpdate<int> value)>;
    using HandleFloats  = std::function<void(Key const& key, ArrayUpdate<float> value)>;
    using HandleStrings = std::function<void(Key const& key, ArrayUpdate<std::string> value)>;

    BasicStore();
    ~BasicStore();

    void add_int_attribute(Key const& key, std::string_view description, Store::HandleInt handler);
    void add_ints_attribute(Key const& key, std::string_view description, HandleInts handler);
    void add_bool_attribute(Key const& key, std::string_view description, Store::HandleBool handler);
    void add_float_attribute(Key const& key, std::string_view description, Store::HandleFloat handler);
    void add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler);
    void add_string_attribute(Key const& key, std::string_view description, Store::HandleString handler);
    void add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler);

    void add_int_attribute(Key const& key, std::string_view description, int preset, Store::HandleInt handler);
    void add_ints_attribute(
        Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler);
    void add_bool_attribute(Key const& key, std::string_view description, bool preset, Store::HandleBool handler);
    void add_float_attribute(Key const& key, std::string_view description, float preset, Store::HandleFloat handler);
    void add_floats_attribute(
        Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler);
    void add_string_attribute(
        Key const& key, std::string_view description, std::string_view preset, Store::HandleString handler);
    void add_strings_attribute(
        Key const& key,
        std::string_view description,
        std::span<std::string const> preset,
        HandleStrings handler);

    void on_done(Store::HandleDone handler);

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    void do_transaction(std::function<void()> transaction_body);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif