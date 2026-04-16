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

#include "basic_store.h"
#include "typed_store_adapter.h"

namespace mlc = miral::live_config;

struct mlc::ConfigAggregator::Self
{
    Self() : adapter{store} {}

    /// Returns a handler that, when called by a source with a raw string value,
    /// stores it in the aggregated store for later typed dispatch.
    auto scalar_accumulator()
    {
        return [this](mlc::Key const& key, std::optional<std::string_view> value)
        {
            if (value)
                store.update_key(key, *value, file_being_loaded);
        };
    }

    /// Returns a handler that, when called by a source with a span of raw string
    /// values, stores each one in the aggregated store for later typed dispatch.
    auto array_accumulator()
    {
        return [this](mlc::Key const& key, std::optional<std::span<std::string const>> values)
        {
            if (values)
                for (auto const& v : *values)
                    store.update_key(key, v, file_being_loaded);
        };
    }

    // store must be declared before adapter so it is constructed first
    BasicStore store;
    TypedStoreAdapter adapter;
    std::filesystem::path file_being_loaded;
    std::vector<Source> sources;
};

mlc::ConfigAggregator::ConfigAggregator()
    : self(std::make_unique<Self>())
{
}

mlc::ConfigAggregator::~ConfigAggregator() = default;

void mlc::ConfigAggregator::add_source(Source&& source)
{
    // Catch up the new source on all attributes already registered
    self->store.foreach_scalar_attribute(
        [&](Key const& key, std::string_view description)
        {
            source.store->add_string_attribute(key, description, self->scalar_accumulator());
        });

    self->store.foreach_array_attribute(
        [&](Key const& key, std::string_view description)
        {
            source.store->add_strings_attribute(key, description, self->array_accumulator());
        });

    self->sources.push_back(std::move(source));
}

void mlc::ConfigAggregator::update_source(Source&& source)
{
    auto const it = std::ranges::find_if(
        self->sources, [&](auto const& s) { return s.file_path == source.file_path; });
    if (it != self->sources.end())
    {
        // Catch up the replacement source on all attributes already registered,
        // mirroring what add_source does for new sources.
        self->store.foreach_scalar_attribute(
            [&](Key const& key, std::string_view description)
            {
                source.store->add_string_attribute(key, description, self->scalar_accumulator());
            });

        self->store.foreach_array_attribute(
            [&](Key const& key, std::string_view description)
            {
                source.store->add_strings_attribute(key, description, self->array_accumulator());
            });

        *it = std::move(source);
    }
}

void mlc::ConfigAggregator::remove_source(std::filesystem::path const& path)
{
    std::erase_if(self->sources, [&](auto const& source) { return source.file_path == path; });
}

void mlc::ConfigAggregator::reload_all()
{
    self->store.do_transaction(
        [&]
        {
            for (auto& source : self->sources)
            {
                self->file_being_loaded = source.file_path;
                source.load();
            }
        });
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, HandleInt handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_int_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_ints_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_bool_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_float_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_floats_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_string_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    for (auto& source : self->sources)
        source.store->add_string_attribute(key, description, self->scalar_accumulator());
    self->adapter.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_strings_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    for (auto& source : self->sources)
        source.store->add_strings_attribute(key, description, self->array_accumulator());
    self->adapter.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->store.on_done(std::move(handler));
}
