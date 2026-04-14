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

#include "typed_store_adapter.h"

#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <charconv>
#include <format>
#include <ranges>
#include <vector>

namespace mlc = miral::live_config;
using ScalarValue = mlc::BasicStore::ScalarValue;
using ArrayValue = mlc::BasicStore::ArrayValue;

namespace
{
template <typename Range> auto join_comma(Range const& strings) -> std::string
{
    auto const sep = std::string_view{", "};
    auto comma_separated = strings | std::views::join_with(sep);

    // LEGACY(24.04) - `std::ranges::to` is not yet widely available, so we need to copy the result into a string
    // manually if it's not available. Remove this workaround once we drop 24.04 support.
#ifdef __cpp_lib_ranges_to_container
    return comma_separated | std::ranges::to<std::string>();
#else
    std::string temp;
    std::ranges::copy(comma_separated, std::back_inserter(temp));

    return temp;
#endif
}
class ParsingError : public std::runtime_error
{
public:
    explicit ParsingError(mlc::Key const& key, std::string_view value) :
        std::runtime_error(std::format("Config key '{}' has invalid value: {}", key, value))
    {
    }
};

class NoValidValuesError : public std::runtime_error
{
public:
    explicit NoValidValuesError(mlc::Key const& key, std::span<std::string const> values) :
        std::runtime_error(
            std::format("All values assigned to config key '{}' are invalid ([{}]).", key, join_comma(values)))
    {
    }
};

template <typename Type>
void process_as(
    std::function<void(mlc::Key const&, std::optional<Type>)> const& handler,
    mlc::Key const& key,
    std::optional<std::string_view> val,
    std::optional<Type> preset)
{
    if (val)
    {
        Type parsed_val{};

        auto const [end, err] = std::from_chars(val->data(), val->data() + val->size(), parsed_val);

        if ((err == std::errc{}) && (end == val->data() + val->size()))
        {
            handler(key, parsed_val);
        }
        else
        {
            BOOST_THROW_EXCEPTION(ParsingError(key, *val));
        }
    }
    else if (preset)
    {
        handler(key, preset);
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template <>
void process_as<bool>(
    std::function<void(mlc::Key const&, std::optional<bool>)> const& handler,
    mlc::Key const& key,
    std::optional<std::string_view> val,
    std::optional<bool> preset)
{
    if (val)
    {
        if (*val == "true")
        {
            handler(key, true);
        }
        else if (*val == "false")
        {
            handler(key, false);
        }
        else
        {
            BOOST_THROW_EXCEPTION(ParsingError(key, *val));
        }
    }
    else if (preset)
    {
        handler(key, preset);
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template <typename Type>
void process_as(
    std::function<void(mlc::Key const&, std::optional<std::span<Type const>>)> const& handler,
    mlc::Key const& key,
    std::span<std::string const> val,
    std::span<Type const> preset)
{
    if (!val.empty())
    {
        std::vector<Type> parsed_vals;

        for (auto const& v : val)
        {
            Type parsed_val{};
            auto const [end, err] = std::from_chars(v.data(), v.data() + v.size(), parsed_val);

            if ((err == std::errc{}) && (end == v.data() + v.size()))
            {
                parsed_vals.push_back(parsed_val);
            }
            else
            {
                mir::log_warning("Config key '%s' has invalid value: %s", key.to_string().c_str(), v.c_str());
            }
        }

        if (parsed_vals.empty())
        {
            BOOST_THROW_EXCEPTION(NoValidValuesError(key, val));
        }

        handler(key, parsed_vals);
    }
    else if (!preset.empty())
    {
        handler(key, preset);
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template <typename T>
auto to_string(T const& thing) -> std::string {
    if constexpr(std::is_same_v<T, std::string_view>)
        return std::string{thing};
    else
        return std::to_string(thing);
}

template <typename Type, typename HandlerType>
void process_or_handle_error(
    HandlerType const& handler,
    mlc::Key const& key,
    std::optional<ScalarValue> const& scalar,
    std::optional<Type> const& preset)
{
    try
    {
        process_as<Type>(handler, key, scalar.transform([](auto const& v) { return v.value; }), preset);
    }
    catch (ParsingError const& pe)
    {
        auto const path =
            scalar.transform([&](auto const& s) { return s.modification_path.string(); }).value_or("never set");

        if (preset)
        {
            mir::log_warning(
                "Parsing error: %s in file %s. Using preset value '%s' instead.",
                pe.what(),
                path.c_str(),
                to_string(*preset).c_str());

            handler(key, preset);
        }
        else
        {
            mir::log_warning(
                "Parsing error: %s in file %s, but no preset value. Using nullopt instead.", pe.what(), path.c_str());
            handler(key, std::nullopt);
        }
    }
    catch (std::exception const& e)
    {
        auto const path =
            scalar.transform([&](auto const& s) { return s.modification_path.string(); }).value_or("never set");
        // `std::string` is required to get a `c_str` from a string view.
        auto const value_str = std::string{scalar.transform([](auto const& s) { return s.value; }).value_or("unset")};

        mir::log_warning(
            "Error processing key '%s' with value '%s' in file '%s': %s",
            key.to_string().c_str(),
            value_str.c_str(),
            path.c_str(),
            e.what());
    }
}

template <typename Type, typename HandlerType>
void process_or_handle_error(
    HandlerType const& handler,
    mlc::Key const& key,
    ArrayValue const& array,
    std::span<Type const>  preset)
{
    try
    {
        process_as<Type>(handler, key, array.values, preset);
    }
    catch (NoValidValuesError const& nvv)
    {
        auto const path = [&]
        {
            if (array.modification_paths.empty())
                return std::string{"never set"};

            return join_comma(
                array.modification_paths | std::views::transform([](auto const& p) { return p.string(); }));
        }();

        if (!preset.empty())
        {
            auto const foo = std::views::transform(preset, [](auto const& e) { return to_string(e); });
            auto const preset_str = join_comma(foo);

            mir::log_warning(
                "Parsing error: %s in file %s. Using preset value(s) '[%s]' instead.",
                nvv.what(),
                path.c_str(),
                preset_str.c_str());

            handler(key, preset);
        }
        else
        {
            mir::log_warning(
                "Parsing error: %s in file %s, but no preset value. Using nullopt instead.", nvv.what(), path.c_str());
            handler(key, std::nullopt);
        }
    }
    catch (std::exception const& e)
    {
        auto const path = [&]
        {
            if (array.modification_paths.empty())
                return std::string{"never set"};

            return join_comma(
                std::ranges::views::transform(array.modification_paths, [](auto const& p) { return p.string(); }));
        }();
        auto const value_str = array.values.empty() ? "unset" : join_comma(array.values);

        mir::log_warning(
            "Error processing key '%s' with values [%s] in file '%s': %s",
            key.to_string().c_str(),
            value_str.c_str(),
            path.c_str(),
            e.what());
    }
}
}

mlc::TypedStoreAdapter::TypedStoreAdapter(BasicStore& store) :
    store_{store}
{
}

void mlc::TypedStoreAdapter::add_int_attribute(
    Key const& key, std::string_view description, Store::HandleInt handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<int>(handler, k, val, std::nullopt);
        });
}

void mlc::TypedStoreAdapter::add_ints_attribute(
    Key const& key, std::string_view description, Store::HandleInts handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler)](Key const& k, ArrayValue val)
        {
            process_or_handle_error<int>(handler, k, val, {});
        });
}

void mlc::TypedStoreAdapter::add_bool_attribute(
    Key const& key, std::string_view description, Store::HandleBool handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<bool>(handler, k, val, std::nullopt);
        });
}

void mlc::TypedStoreAdapter::add_float_attribute(
    Key const& key, std::string_view description, Store::HandleFloat handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<float>(handler, k, val, std::nullopt);
        });
}

void mlc::TypedStoreAdapter::add_floats_attribute(
    Key const& key, std::string_view description, Store::HandleFloats handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler)](Key const& k, ArrayValue val)
        {
            process_or_handle_error<float>(handler, k, val, {});
        });
}

void mlc::TypedStoreAdapter::add_string_attribute(
    Key const& key, std::string_view description, Store::HandleString handler)
{
    store_.add_scalar_attribute(
        key,
        description,
        [handler = std::move(handler)](Key const& k, std::optional<ScalarValue> val)
        {
            handler(k, val.transform([](auto const s) { return s.value; }));
        });
}

void mlc::TypedStoreAdapter::add_strings_attribute(
    Key const& key, std::string_view description, Store::HandleStrings handler)
{
    store_.add_array_attribute(
        key,
        description,
        [handler = std::move(handler)](Key const& k, ArrayValue val)
        {
            auto const maybe_values =
                val.values.empty() ? std::nullopt : std::optional<std::span<std::string const>>{val.values};
            handler(k, maybe_values);
        });
}

void mlc::TypedStoreAdapter::add_int_attribute(
    Key const& key, std::string_view description, int preset, Store::HandleInt handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<int>(handler, k, val, preset);
        });
}

void mlc::TypedStoreAdapter::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, Store::HandleInts handler)
{
    store_.add_array_attribute(
        key,
        description,
        [handler = std::move(handler), copied_preset = std::vector<int>{preset.begin(), preset.end()}](
            Key const& k, ArrayValue val) { process_or_handle_error<int>(handler, k, val, copied_preset); });
}

void mlc::TypedStoreAdapter::add_bool_attribute(
    Key const& key, std::string_view description, bool preset, Store::HandleBool handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<bool>(handler, k, val, preset);
        });
}

void mlc::TypedStoreAdapter::add_float_attribute(
    Key const& key, std::string_view description, float preset, Store::HandleFloat handler)
{

    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<ScalarValue> val)
        {
            process_or_handle_error<float>(handler, k, val, preset);
        });
}

void mlc::TypedStoreAdapter::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, Store::HandleFloats handler)
{
    store_.add_array_attribute(
        key,
        description,
        [handler = std::move(handler), copied_preset = std::vector<float>{preset.begin(), preset.end()}](
            Key const& k, ArrayValue val) { process_or_handle_error<float>(handler, k, val, copied_preset); });
}

void mlc::TypedStoreAdapter::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, Store::HandleString handler)
{
    store_.add_scalar_attribute(
        key,
        description,
        [handler = std::move(handler), preset](Key const& k, std::optional<ScalarValue> val)
        {
            auto const maybe_value =
                val.transform([](auto const& s) { return std::string_view{s.value}; }).value_or(preset);
            handler(k, maybe_value);
        });
}

void mlc::TypedStoreAdapter::add_strings_attribute(
    Key const& key, std::string_view description, std::span<std::string const> preset, Store::HandleStrings handler)
{
    store_.add_array_attribute(
        key,
        description,
        [handler = std::move(handler), copied_preset = std::vector<std::string>{preset.begin(), preset.end()}](
            Key const& k, ArrayValue val) { handler(k, val.values.empty() ? copied_preset : val.values); });
}
