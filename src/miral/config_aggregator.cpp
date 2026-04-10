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

#include <type_traits>

namespace mlc = miral::live_config;

struct mlc::ConfigAggregator::Self
{
    explicit Self() = default;

    /// Returns a handler suitable for registering on the parser that, when
    /// called with a parsed scalar value, forwards it into the aggregated store.
    template <typename T>
    auto accumulate_scalar()
    {
        return [this](mlc::Key const& key, std::optional<T> value)
        {
            if (!value)
                return;
            if constexpr (std::is_same_v<T, int>)
                store.update_int_key(key, *value, file_being_loaded);
            else if constexpr (std::is_same_v<T, bool>)
                store.update_bool_key(key, *value, file_being_loaded);
            else if constexpr (std::is_same_v<T, float>)
                store.update_float_key(key, *value, file_being_loaded);
            else if constexpr (std::is_same_v<T, std::string_view>)
                store.update_string_key(key, *value, file_being_loaded);
        };
    }

    /// Returns a handler suitable for registering on the parser that, when
    /// called with a parsed array span, appends the values into the aggregated
    /// store, or clears the array if the span is empty.
    template <typename T>
    auto accumulate_array()
    {
        return [this](mlc::Key const& key, std::optional<std::span<T const>> values)
        {
            if (!values)
                return;
            if (values->empty())
            {
                store.clear_array(key, file_being_loaded);
                return;
            }
            if constexpr (std::is_same_v<T, int>)
                store.update_int_array_key(key, *values, file_being_loaded);
            else if constexpr (std::is_same_v<T, float>)
                store.update_float_array_key(key, *values, file_being_loaded);
            else if constexpr (std::is_same_v<T, std::string>)
                store.update_string_array_key(key, *values, file_being_loaded);
        };
    }

    mlc::BasicStore store;
    std::filesystem::path file_being_loaded;
    std::vector<Source> sources;
};

mlc::ConfigAggregator::ConfigAggregator()
    : self(std::make_unique<Self>())
{
}

mlc::ConfigAggregator::~ConfigAggregator() = default;

namespace
{
template <typename T, typename... Args>
void add_typed_attribute(mlc::Store& store, Args&&... args)
{
    if constexpr (std::is_same_v<T, int>)
        store.add_int_attribute(std::forward<Args>(args)...);
    else if constexpr (std::is_same_v<T, bool>)
        store.add_bool_attribute(std::forward<Args>(args)...);
    else if constexpr (std::is_same_v<T, float>)
        store.add_float_attribute(std::forward<Args>(args)...);
    else if constexpr (std::is_same_v<T, std::string_view>)
        store.add_string_attribute(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
void add_typed_array_attribute(mlc::Store& store, Args&&... args)
{
    if constexpr (std::is_same_v<T, int>)
        store.add_ints_attribute(std::forward<Args>(args)...);
    else if constexpr (std::is_same_v<T, float>)
        store.add_floats_attribute(std::forward<Args>(args)...);
    else if constexpr (std::is_same_v<T, std::string>)
        store.add_strings_attribute(std::forward<Args>(args)...);
}

template <typename T, typename F>
void register_array_accumulator(mlc::BasicStore& store, mlc::Store& source_store, F&& accumulator)
{
    auto const add = [&](mlc::Key const& key, std::string_view description, auto preset)
    {
        if (preset)
            add_typed_array_attribute<T>(source_store, key, description, *preset, accumulator);
        else
            add_typed_array_attribute<T>(source_store, key, description, accumulator);
    };

    if constexpr (std::is_same_v<T, int>)
        store.foreach_ints_attribute(add);
    else if constexpr (std::is_same_v<T, float>)
        store.foreach_floats_attribute(add);
    else if constexpr (std::is_same_v<T, std::string>)
        store.foreach_strings_attribute(add);
}

template <typename T, typename F>
void register_scalar_accumulator(mlc::BasicStore& store, mlc::Store& source_store, F&& accumulator)
{
    auto const add = [&](mlc::Key const& key, std::string_view description, auto preset)
    {
        if (preset)
            add_typed_attribute<T>(source_store, key, description, *preset, accumulator);
        else
            add_typed_attribute<T>(source_store, key, description, accumulator);
    };

    if constexpr (std::is_same_v<T, int>)
        store.foreach_int_attribute(add);
    else if constexpr (std::is_same_v<T, bool>)
        store.foreach_bool_attribute(add);
    else if constexpr (std::is_same_v<T, float>)
        store.foreach_float_attribute(add);
    else if constexpr (std::is_same_v<T, std::string_view>)
        store.foreach_string_attribute(add);
}
}

void mlc::ConfigAggregator::add_source(Source&& source)
{
    register_scalar_accumulator<int>(self->store, *source.store, self->accumulate_scalar<int>());
    register_scalar_accumulator<float>(self->store, *source.store, self->accumulate_scalar<float>());
    register_scalar_accumulator<bool>(self->store, *source.store, self->accumulate_scalar<bool>());
    register_scalar_accumulator<std::string_view>(self->store, *source.store, self->accumulate_scalar<std::string_view>());

    register_array_accumulator<int>(self->store, *source.store, self->accumulate_array<int>());
    register_array_accumulator<float>(self->store, *source.store, self->accumulate_array<float>());
    register_array_accumulator<std::string>(self->store, *source.store, self->accumulate_array<std::string>());

    self->sources.push_back(std::move(source));
}

void mlc::ConfigAggregator::remove_source(std::filesystem::path const& path)
{
    std::erase_if(self->sources, [&](auto const& source) { return source.file_path == path; });
}

void mlc::ConfigAggregator::reload_all()
{
    self->store.clear_values();

    std::vector<std::filesystem::path> paths;
    paths.reserve(self->sources.size());
    for (auto& source : self->sources)
    {
        self->file_being_loaded = source.file_path;
        paths.push_back(source.file_path);
        source.load();
    }

    self->store.done(paths);
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, HandleInt handler)
{
    for (auto& source : self->sources)
        source.store->add_int_attribute(key, description, self->accumulate_scalar<int>());
    self->store.add_int_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    for (auto& source : self->sources)
        source.store->add_int_attribute(key, description, preset, self->accumulate_scalar<int>());
    self->store.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    for (auto& source : self->sources)
        source.store->add_ints_attribute(key, description, self->accumulate_array<int>());
    self->store.add_ints_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    for (auto& source : self->sources)
        source.store->add_ints_attribute(key, description, preset, self->accumulate_array<int>());
    self->store.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    for (auto& source : self->sources)
        source.store->add_bool_attribute(key, description, self->accumulate_scalar<bool>());
    self->store.add_bool_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    for (auto& source : self->sources)
        source.store->add_bool_attribute(key, description, preset, self->accumulate_scalar<bool>());
    self->store.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    for (auto& source : self->sources)
        source.store->add_float_attribute(key, description, self->accumulate_scalar<float>());
    self->store.add_float_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    for (auto& source : self->sources)
        source.store->add_float_attribute(key, description, preset, self->accumulate_scalar<float>());
    self->store.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    for (auto& source : self->sources)
        source.store->add_floats_attribute(key, description, self->accumulate_array<float>());
    self->store.add_floats_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    for (auto& source : self->sources)
        source.store->add_floats_attribute(key, description, preset, self->accumulate_array<float>());
    self->store.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->accumulate_scalar<std::string_view>());
    self->store.add_string_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, preset, self->accumulate_scalar<std::string_view>());
    self->store.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->accumulate_array<std::string>());
    self->store.add_strings_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, preset, self->accumulate_array<std::string>());
    self->store.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->store.on_done(std::move(handler));
}
