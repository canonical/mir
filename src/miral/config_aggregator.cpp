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

namespace mlc = miral::live_config;

struct mlc::ConfigAggregator::Self: BasicStore
{
    /// Returns a handler that, when called by a source with a raw string value,
    /// stores it in the aggregated store for later typed dispatch.
    auto scalar_accumulator()
    {
        return [this](mlc::Key const& key, std::optional<std::string_view> value)
        {
            if (value)
                update_key(key, *value, file_being_loaded);
        };
    }

    /// Returns a handler that, when called by a source with a span of raw string
    /// values, stores each one in the aggregated store for later typed dispatch.
    auto array_accumulator()
    {
        return [this](mlc::Key const& key, std::optional<std::span<std::string const>> values)
        {
            set_array(key, values, file_being_loaded);
        };
    }

    // store must be declared before adapter so it is constructed first
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
    self->sources.push_back(std::move(source));
}

void mlc::ConfigAggregator::update_source(Source&& source)
{
    auto const it = std::ranges::find_if(
        self->sources, [&](auto const& s) { return s.file_path == source.file_path; });

    if (it != self->sources.end())
        *it = std::move(source);
}

void mlc::ConfigAggregator::remove_source(std::filesystem::path const& path)
{
    std::erase_if(self->sources, [&](auto const& source) { return source.file_path == path; });
}

void mlc::ConfigAggregator::reload_all()
{
    self->do_transaction(
        [&]
        {
            for (auto& source : self->sources)
            {
                self->file_being_loaded = source.file_path;
                // Catch up the new source on all attributes already registered
                self->foreach_attribute(
                    [&](Key const& key, std::string_view description, std::optional<std::string> const& preset)
                    {
                        if (preset)
                            source.store->add_string_attribute(key, description, *preset, self->scalar_accumulator());
                        else
                            source.store->add_string_attribute(key, description, self->scalar_accumulator());
                    });

                self->foreach_array_attribute(
                    [&](Key const& key,
                        std::string_view description,
                        std::span<std::string const> current_value,
                        std::optional<std::span<std::string const>> preset)
                    {
                        if (preset)
                            source.store->add_strings_attribute(
                                key, description, *preset, self->array_accumulator(), current_value);
                        else
                            source.store->add_strings_attribute(
                                key, description, self->array_accumulator(), current_value);
                    });

                source.load();
            }
        });
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, HandleInt handler)
{
    self->add_int_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_int_attribute(
    live_config::Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, HandleInts handler)
{
    self->add_ints_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_ints_attribute(
    live_config::Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, HandleBool handler)
{
    self->add_bool_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_bool_attribute(
    live_config::Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    self->add_float_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_float_attribute(
    live_config::Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, HandleFloats handler)
{
    self->add_floats_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_floats_attribute(
    live_config::Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, HandleString handler)
{
    self->add_string_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_string_attribute(
    live_config::Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key, std::string_view description, HandleStrings handler)
{
    self->add_strings_attribute(key, description, std::move(handler));
}

void mlc::ConfigAggregator::add_strings_attribute(
    live_config::Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->add_strings_attribute(key, description, preset, std::move(handler));
}

void miral::live_config::ConfigAggregator::add_strings_attribute(
    Key const& key, std::string_view description, HandleStrings handler, std::span<std::string const> initial_values)
{
    self->add_strings_attribute(key, description, std::move(handler), initial_values);
}

void miral::live_config::ConfigAggregator::add_strings_attribute(
    Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler,
    std::span<std::string const> initial_values)
{
    self->add_strings_attribute(key, description, preset, std::move(handler), initial_values);
}

void mlc::ConfigAggregator::on_done(HandleDone handler)
{
    self->on_done(std::move(handler));
}
