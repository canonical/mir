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
#include "live_config_basic_store.h"

#include <miral/live_config_parsing_store.h>

#include <ranges>

namespace mlc = miral::live_config;

struct mlc::ConfigAggregator::Self
{
    explicit Self(mlc::ParsingStore& parser) :
        parser{parser}
    {
    }

    /// Returns a handler suitable for registering on the parser that, when
    /// called with a parsed scalar value, forwards it into the aggregated store
    /// using the typed path (no string conversion).
    template <typename T>
    auto accumulate_scalar()
    {
        return [this](mlc::Key const& key, std::optional<T> value)
        {
            if (value)
                store.update_typed_value(key, *value, file_being_loaded);
        };
    }

    /// Returns a handler suitable for registering on the parser that, when
    /// called with a parsed array span, appends each element into the
    /// aggregated store using the typed path, or clears the array if the span
    /// is empty.
    template <typename T>
    auto accumulate_array()
    {
        return [this](mlc::Key const& key, std::optional<std::span<T const>> values)
        {
            if (values)
            {
                if (values->empty())
                    store.clear_array(key);
                else
                    for (auto const& v : *values)
                        store.update_typed_array_value(key, v, file_being_loaded);
            }
        };
    }

    mlc::ParsingStore& parser;
    mlc::BasicStore store;
    std::filesystem::path file_being_loaded;
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
    {
        self->file_being_loaded = path;
        self->parser.load_file(istream_ref.get(), path);
    }

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
    self->parser.add_int_attribute(key, description, self->accumulate_scalar<int>());
    self->store.add_int_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->parser.add_int_attribute(key, description, preset, self->accumulate_scalar<int>());
    self->store.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description, self->accumulate_array<int>());
    self->store.add_ints_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->parser.add_ints_attribute(key, description, preset, self->accumulate_array<int>());
    self->store.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description, self->accumulate_scalar<bool>());
    self->store.add_bool_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->parser.add_bool_attribute(key, description, preset, self->accumulate_scalar<bool>());
    self->store.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description, self->accumulate_scalar<float>());
    self->store.add_float_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->parser.add_float_attribute(key, description, preset, self->accumulate_scalar<float>());
    self->store.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description, self->accumulate_array<float>());
    self->store.add_floats_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->parser.add_floats_attribute(key, description, preset, self->accumulate_array<float>());
    self->store.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    self->parser.add_string_attribute(key, description, self->accumulate_scalar<std::string_view>());
    self->store.add_string_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->parser.add_string_attribute(key, description, preset, self->accumulate_scalar<std::string_view>());
    self->store.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    self->parser.add_strings_attribute(key, description, self->accumulate_array<std::string>());
    self->store.add_strings_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->parser.add_strings_attribute(key, description, preset, self->accumulate_array<std::string>());
    self->store.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->store.on_done(std::move(handler));
}
