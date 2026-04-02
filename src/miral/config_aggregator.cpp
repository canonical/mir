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

#include "miral/config_aggregator.h"
#include "miral/live_config.h"

#include <mir/log.h>
#include <miral/live_config_parsing_store.h>

#include <algorithm>
#include <filesystem>
#include <list>
#include <map>
#include <ranges>
#include <variant>

namespace mlc = miral::live_config;

namespace
{
/// Typed storage for a single scalar config value
struct Scalar
{
    template <typename T>
    Scalar(T const& v) :
        value{std::variant<int, float, bool, std::string>{v}}
    {
    }

    Scalar(std::string_view v) :
        value{std::variant<int, float, bool, std::string>{std::string{v}}}
    {
    }

    template <typename T> auto get() const -> T const&
    {
        return std::get<T>(value);
    }

    std::variant<int, float, bool, std::string> value;
};

using Array = std::variant<std::vector<int>, std::vector<float>, std::vector<std::string>>;

template <typename T> auto value_to_string(T const& v) -> std::string
{
    if constexpr (std::is_same_v<T, std::string>)
        return v;
    else if constexpr (std::is_same_v<T, bool>)
        return v ? "true" : "false";
    else
        return std::to_string(v);
}

struct TypedConfigState
{
    struct AttributeDetails
    {
        std::function<void(mlc::Key const&, std::optional<Scalar> const&)> invoke;
        std::string description;
        std::optional<Scalar> preset;
        std::optional<Scalar> value;
    };

    struct ArrayAttributeDetails
    {
        std::function<void(mlc::Key const&, std::optional<Array> const&)> invoke;
        std::string description;
        std::optional<Array> preset;
        std::optional<Array> values;
        std::vector<std::filesystem::path> modification_locations;
        bool explicitly_cleared = false;
    };

    void add_scalar_attribute(mlc::Key const& key, AttributeDetails details)
    {
        if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
        attribute_handlers.emplace(key, std::move(details));
    }

    void add_array_attribute(mlc::Key const& key, ArrayAttributeDetails details)
    {
        if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
        array_attribute_handlers.emplace(key, std::move(details));
    }

    void clear()
    {
        for (auto& [_, details] : attribute_handlers)
            details.value = std::nullopt;

        for (auto& [_, details] : array_attribute_handlers)
        {
            details.values = std::nullopt;
            details.modification_locations.clear();
            details.explicitly_cleared = false;
        }
    }

    template <typename T> void update_scalar_value(mlc::Key const& key, T const& value)
    {
        attribute_handlers.at(key).value = Scalar{value};
    }

    void clear_scalar_value(mlc::Key const& key)
    {
        attribute_handlers.at(key).value = std::nullopt;
    }

    template <typename T> void update_array_value(mlc::Key const& key, std::span<T const> values)
    {
        auto& details = array_attribute_handlers.at(key);
        if (!details.values)
            details.values = std::vector<T>{};
        auto& vec = std::get<std::vector<T>>(*details.values);
        vec.insert(vec.end(), values.begin(), values.end());
        details.explicitly_cleared = false;
    }

    void clear_array_value(mlc::Key const& key)
    {
        auto& details = array_attribute_handlers.at(key);
        details.values = std::nullopt;
        details.explicitly_cleared = true;
    }

    void call_attribute_handlers() const
    {
        for (auto const& [key, details] : attribute_handlers)
            try
            {
                auto const& effective_value = details.value.or_else([&] { return details.preset; });
                details.invoke(key, effective_value);
            }
            catch (std::exception const& e)
            {
                auto const to_string = [&](auto const& v)
                {
                    return std::visit([&](auto const& v) { return value_to_string(v); }, v.value);
                };

                auto const value_str = details.value.transform(to_string).value_or("");

                mir::log_warning(
                    "Error processing scalar '%s' with value '%s': %s",
                    key.to_string().c_str(),
                    value_str.c_str(),
                    e.what());
            }

        for (auto const& [key, details] : array_attribute_handlers)
            try
            {
                // If the user didn't explicitly clear the array and no values were provided,
                // use the preset values.
                if (!details.explicitly_cleared && !details.values.has_value())
                    details.invoke(key, details.preset);
                else
                    details.invoke(key, details.values);
            }
            catch (std::exception const& e)
            {
                auto const values_to_str = [](auto const& values)
                {
                    // auto parameter allows us to use the same
                    // visitor for all variants of ArrayElements
                    return std::visit(
                        [](auto const& span)
                        {
                            std::string temp;
                            std::ranges::copy(
                                span | std::views::transform([](auto const& v) { return value_to_string(v); })
                                    | std::views::join_with(','),
                                std::back_inserter(temp));

                            return temp;
                        },
                        values);
                };

                auto const values_str = details.values.transform(values_to_str).value_or(std::string{});

                std::string modification_locations_str = [&]
                {
                    std::string temp;
                    std::ranges::copy(
                        details.modification_locations | std::views::transform([](auto const& p) { return p.string(); })
                            | std::views::join_with(','),
                        std::back_inserter(temp));

                    return temp;
                }();

                mir::log_warning(
                    "Error processing array '%s' with values [%s] modified in files [%s]: %s",
                    key.to_string().c_str(),
                    values_str.c_str(),
                    modification_locations_str.c_str(),
                    e.what());
            }
    }

    void call_done_handlers(std::string const& paths) const
    {
        for (auto const& h : done_handlers)
            try
            {
                h();
            }
            catch (std::exception const& e)
            {
                mir::log_warning("Error processing file(s) [%s]: %s", paths.c_str(), e.what());
            }
    }

    void on_done(mlc::Store::HandleDone handler)
    {
        done_handlers.push_back(std::move(handler));
    }

    std::map<mlc::Key, AttributeDetails> attribute_handlers;
    std::map<mlc::Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<mlc::Store::HandleDone> done_handlers;
};

} // namespace

struct mlc::ConfigAggregator::Self
{
    explicit Self(mlc::ParsingStore& parser) :
        parser{parser}
    {
    }

    // Captures the type of the value so we can retreive it later on
    template <typename T>
    void register_scalar_attribute(
        mlc::Key const& key, std::string_view description, std::optional<Scalar> preset, auto user_handler)
    {
        config_state.add_scalar_attribute(
            key,
            {[user_handler](mlc::Key const& key, std::optional<Scalar> const& val)
             {
                 auto const maybe_val = val.transform([](Scalar const& s) { return s.get<T>(); });
                 user_handler(key, maybe_val);
             },
             std::string{description},
             std::move(preset),
             std::nullopt});
    }

    template <typename T> auto update_or_clear_scalar() -> std::function<void(mlc::Key const&, std::optional<T>)>
    {
        return [&](mlc::Key const& key, std::optional<T> value)
        {
            if (value)
                config_state.update_scalar_value(key, *value);
            else
                config_state.clear_scalar_value(key);
        };
    }

    template <typename T>
    void register_array_attribute(
        mlc::Key const& key, std::string_view description, std::optional<std::span<T const>> preset, auto user_handler)
    {
        config_state.add_array_attribute(
            key,
            {[user_handler](mlc::Key const& key, std::optional<Array> const& val)
             {
                 auto const maybe_val =
                     val.transform([](Array const& array) { return std::get<std::vector<T>>(array); });

                 user_handler(key, maybe_val);
             },
             std::string{description},
             preset.transform([](std::span<T const> s) { return Array{std::vector<T>{s.begin(), s.end()}}; }),
             std::nullopt,
             {},
             false});
    }

    template <typename T>
    auto update_or_clear_array() -> std::function<void(mlc::Key const&, std::optional<std::span<T const>>)>
    {
        return [&](mlc::Key const& key, std::optional<std::span<T const>> value)
        {
            if (value)
                config_state.update_array_value(key, *value);
            else
                config_state.clear_array_value(key);
        };
    }

    mlc::ParsingStore& parser;
    TypedConfigState config_state;
};

mlc::ConfigAggregator::ConfigAggregator(ParsingStore& parser) :
    self{std::make_unique<Self>(parser)}
{
}

mlc::ConfigAggregator::~ConfigAggregator() = default;

void mlc::ConfigAggregator::load_all(
    std::span<std::pair<std::reference_wrapper<std::istream>, std::filesystem::path>> const& config_files)
{
    self->config_state.clear();

    for (auto& [istream_ref, path] : config_files)
        self->parser.load_file(istream_ref.get(), path);

    self->config_state.call_attribute_handlers();

    std::string paths_str;
    std::ranges::copy(
        config_files | std::views::transform([](auto const& p) { return p.second.string(); })
            | std::views::join_with(','),
        std::back_inserter(paths_str));

    self->config_state.call_done_handlers(paths_str);
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, HandleInt handler)
{
    self->parser.add_int_attribute(key, description, self->update_or_clear_scalar<int>());
    self->register_scalar_attribute<int>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->parser.add_int_attribute(key, description, preset, self->update_or_clear_scalar<int>());
    self->register_scalar_attribute<int>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description, self->update_or_clear_array<int>());
    self->register_array_attribute<int>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description, preset, self->update_or_clear_array<int>());
    self->register_array_attribute<int>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description, self->update_or_clear_scalar<bool>());
    self->register_scalar_attribute<bool>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description, preset, self->update_or_clear_scalar<bool>());
    self->register_scalar_attribute<bool>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description, self->update_or_clear_scalar<float>());
    self->register_scalar_attribute<float>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description, preset, self->update_or_clear_scalar<float>());
    self->register_scalar_attribute<float>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description, self->update_or_clear_array<float>());
    self->register_array_attribute<float>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description, preset, self->update_or_clear_array<float>());
    self->register_array_attribute<float>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    self->parser.add_string_attribute(key, description, self->update_or_clear_scalar<std::string_view>());
    // Notice that we use std::string here, not std::string_view, we store the
    // value as an owned std::string and pass it to handlers as a view.
    self->register_scalar_attribute<std::string>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->parser.add_string_attribute(key, description, preset, self->update_or_clear_scalar<std::string_view>());
    self->register_scalar_attribute<std::string>(key, description, preset, handler);
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    // Unlike the scalar string case, the interface dictates that handlers
    // receive spans of std::string.
    self->parser.add_strings_attribute(key, description, self->update_or_clear_array<std::string>());
    self->register_array_attribute<std::string>(key, description, std::nullopt, handler);
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->parser.add_strings_attribute(key, description, preset, self->update_or_clear_array<std::string>());
    self->register_array_attribute<std::string>(key, description, preset, handler);
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->config_state.on_done(std::move(handler));
}
