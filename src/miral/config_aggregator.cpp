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
#include "miral/live_config_basic_store.h"
#include "miral/live_config_parsing_store.h"

#include <ranges>

namespace mlc = miral::live_config;

struct mlc::ConfigAggregator::Self
{
    explicit Self(mlc::ParsingStore& parser) :
        parser{parser}
    {
    }

    mlc::ParsingStore& parser;
    mlc::BasicStore store;
};

mlc::ConfigAggregator::ConfigAggregator(ParsingStore& parser) :
    self{std::make_unique<Self>(parser)}
{
}

mlc::ConfigAggregator::~ConfigAggregator() = default;

void mlc::ConfigAggregator::load_all(
    std::span<std::pair<std::reference_wrapper<std::istream>, std::filesystem::path>> const& config_files)
{
    self->store.clear_values();

    for (auto& [istream_ref, path] : config_files)
        self->parser.load_file(istream_ref.get(), path);

    self->store.call_attribute_handlers();

    std::string paths_str;
    std::ranges::copy(
        config_files | std::views::transform([](auto const& p) { return p.second.string(); })
            | std::views::join_with(','),
        std::back_inserter(paths_str));

    self->store.call_done_handlers(paths_str);
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, HandleInt handler)
{
    self->parser.add_int_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<int> value)
        {
            if (value)
                store.update_scalar_value(key, std::to_string(*value), {});
        });
    self->store.add_int_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->parser.add_int_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<int> value)
        {
            if (value)
                store.update_scalar_value(key, std::to_string(*value), {});
        });
    self->store.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<std::span<int const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const v : *values)
                        store.update_array_value(key, std::to_string(v), {});
            }
        });
    self->store.add_ints_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<std::span<int const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const v : *values)
                        store.update_array_value(key, std::to_string(v), {});
            }
        });
    self->store.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<bool> value)
        {
            if (value)
                store.update_scalar_value(key, *value ? "true" : "false", {});
        });
    self->store.add_bool_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<bool> value)
        {
            if (value)
                store.update_scalar_value(key, *value ? "true" : "false", {});
        });
    self->store.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<float> value)
        {
            if (value)
                store.update_scalar_value(key, std::to_string(*value), {});
        });
    self->store.add_float_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<float> value)
        {
            if (value)
                store.update_scalar_value(key, std::to_string(*value), {});
        });
    self->store.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<std::span<float const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const v : *values)
                        store.update_array_value(key, std::to_string(v), {});
            }
        });
    self->store.add_floats_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<std::span<float const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const v : *values)
                        store.update_array_value(key, std::to_string(v), {});
            }
        });
    self->store.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    self->parser.add_string_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<std::string_view> value)
        {
            if (value)
                store.update_scalar_value(key, *value, {});
        });
    self->store.add_string_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->parser.add_string_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<std::string_view> value)
        {
            if (value)
                store.update_scalar_value(key, *value, {});
        });
    self->store.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    self->parser.add_strings_attribute(key, description,
        [&store = self->store](Key const& key, std::optional<std::span<std::string const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const& v : *values)
                        store.update_array_value(key, v, {});
            }
        });
    self->store.add_strings_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->parser.add_strings_attribute(key, description, preset,
        [&store = self->store](Key const& key, std::optional<std::span<std::string const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array_value(key);
                else
                    for (auto const& v : *values)
                        store.update_array_value(key, v, {});
            }
        });
    self->store.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->store.on_done(std::move(handler));
}