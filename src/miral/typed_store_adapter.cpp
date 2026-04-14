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

#include <charconv>
#include <format>
#include <vector>

namespace mlc = miral::live_config;

namespace
{
template<typename Type>
void process_as(
    std::function<void(mlc::Key const&, std::optional<Type>)> const& handler,
    mlc::Key const& key,
    std::optional<std::string_view> val)
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
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), *val));
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template<>
void process_as<bool>(
    std::function<void(mlc::Key const&, std::optional<bool>)> const& handler,
    mlc::Key const& key,
    std::optional<std::string_view> val)
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
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), *val));
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template<typename Type>
void process_as(
    std::function<void(mlc::Key const&, std::optional<std::span<Type>>)> const& handler,
    mlc::Key const& key,
    std::optional<std::span<std::string const>> val)
{
    if (val)
    {
        std::vector<Type> parsed_vals;

        for (auto const& v : *val)
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

        handler(key, parsed_vals);
    }
    else
    {
        handler(key, std::nullopt);
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
        [handler = std::move(handler)](Key const& k, std::optional<std::string_view> val)
        {
            process_as<int>(handler, k, val);
        });
}

void mlc::TypedStoreAdapter::add_ints_attribute(
    Key const& key, std::string_view description, Store::HandleInts handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<std::span<std::string const>> val)
        {
            process_as<int>(handler, k, val);
        });
}

void mlc::TypedStoreAdapter::add_bool_attribute(
    Key const& key, std::string_view description, Store::HandleBool handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<std::string_view> val)
        {
            process_as<bool>(handler, k, val);
        });
}

void mlc::TypedStoreAdapter::add_float_attribute(
    Key const& key, std::string_view description, Store::HandleFloat handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<std::string_view> val)
        {
            process_as<float>(handler, k, val);
        });
}

void mlc::TypedStoreAdapter::add_floats_attribute(
    Key const& key, std::string_view description, Store::HandleFloats handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler)](Key const& k, std::optional<std::span<std::string const>> val)
        {
            process_as<float>(handler, k, val);
        });
}

void mlc::TypedStoreAdapter::add_string_attribute(
    Key const& key, std::string_view description, Store::HandleString handler)
{
    store_.add_scalar_attribute(key, description, std::move(handler));
}

void mlc::TypedStoreAdapter::add_strings_attribute(
    Key const& key, std::string_view description, Store::HandleStrings handler)
{
    store_.add_array_attribute(key, description, std::move(handler));
}

void mlc::TypedStoreAdapter::add_int_attribute(
    Key const& key, std::string_view description, int preset, Store::HandleInt handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<std::string_view> val)
        {
            if (val)
                process_as<int>(handler, k, val);
            else
                handler(k, preset);
        });
}

void mlc::TypedStoreAdapter::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, Store::HandleInts handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler), preset_vec = std::vector<int>(preset.begin(), preset.end())](
            Key const& k, std::optional<std::span<std::string const>> val)
        {
            if (val)
                process_as<int>(handler, k, val);
            else
                handler(k, std::span<int const>{preset_vec});
        });
}

void mlc::TypedStoreAdapter::add_bool_attribute(
    Key const& key, std::string_view description, bool preset, Store::HandleBool handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<std::string_view> val)
        {
            if (val)
                process_as<bool>(handler, k, val);
            else
                handler(k, preset);
        });
}

void mlc::TypedStoreAdapter::add_float_attribute(
    Key const& key, std::string_view description, float preset, Store::HandleFloat handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset](Key const& k, std::optional<std::string_view> val)
        {
            if (val)
                process_as<float>(handler, k, val);
            else
                handler(k, preset);
        });
}

void mlc::TypedStoreAdapter::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, Store::HandleFloats handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler), preset_vec = std::vector<float>(preset.begin(), preset.end())](
            Key const& k, std::optional<std::span<std::string const>> val)
        {
            if (val)
                process_as<float>(handler, k, val);
            else
                handler(k, std::span<float const>{preset_vec});
        });
}

void mlc::TypedStoreAdapter::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, Store::HandleString handler)
{
    store_.add_scalar_attribute(key, description,
        [handler = std::move(handler), preset_str = std::string{preset}](
            Key const& k, std::optional<std::string_view> val)
        {
            handler(k, val ? val : std::optional<std::string_view>{preset_str});
        });
}

void mlc::TypedStoreAdapter::add_strings_attribute(
    Key const& key, std::string_view description, std::span<std::string const> preset, Store::HandleStrings handler)
{
    store_.add_array_attribute(key, description,
        [handler = std::move(handler), preset_vec = std::vector<std::string>(preset.begin(), preset.end())](
            Key const& k, std::optional<std::span<std::string const>> val)
        {
            handler(k, val ? val : std::optional<std::span<std::string const>>{preset_vec});
        });
}
