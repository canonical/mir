/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "nested_display_configuration.h"
#include "host_connection.h"

#include "mir/graphics/pixel_format_utils.h"
#include "mir/raii.h"

#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_blob.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>


namespace mg = mir::graphics;
namespace mgn = mg::nested;

namespace
{
auto copy_config(MirDisplayConfiguration* conf) -> std::shared_ptr<MirDisplayConfiguration>
{
    auto const blob = mir::raii::deleter_for(
        mir_blob_from_display_configuration(conf),
        [] (MirBlob* b) { mir_blob_release(b); });

    return std::shared_ptr<MirDisplayConfiguration>{
        mir_blob_to_display_configuration(blob.get()),
        [] (MirDisplayConfiguration* c) { if (c) mir_display_config_destroy(c); }};
}
}

mgn::NestedDisplayConfiguration::NestedDisplayConfiguration(
    std::shared_ptr<MirDisplayConfiguration> const& display_config)
    : display_config{display_config}
{
}

mgn::NestedDisplayConfiguration::NestedDisplayConfiguration(
    NestedDisplayConfiguration const& other)
    : mg::DisplayConfiguration(),
      display_config{copy_config(other.display_config.get())}
{
    std::lock_guard<std::mutex> lock{other.local_config_mutex};
    local_config = other.local_config;
}

void mgn::NestedDisplayConfiguration::for_each_card(
    std::function<void(DisplayConfigurationCard const&)> f) const
{
    std::for_each(
        display_config->cards,
        display_config->cards+display_config->num_cards,
        [&f](MirDisplayCard const& mir_card)
        {
            f({DisplayConfigurationCardId(mir_card.card_id), mir_card.max_simultaneous_outputs});
        });
}

void mgn::NestedDisplayConfiguration::for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const
{
    std::for_each(
        display_config->outputs,
        display_config->outputs+display_config->num_outputs,
        [&f, this](MirDisplayOutput const& mir_output)
        {
            std::vector<MirPixelFormat> formats;
            formats.reserve(mir_output.num_output_formats);
            for (auto p = mir_output.output_formats; p != mir_output.output_formats+mir_output.num_output_formats; ++p)
                formats.push_back(MirPixelFormat(*p));

            std::vector<DisplayConfigurationMode> modes;
            modes.reserve(mir_output.num_modes);
            for (auto p = mir_output.modes; p != mir_output.modes+mir_output.num_modes; ++p)
                modes.push_back(DisplayConfigurationMode{{p->horizontal_resolution, p->vertical_resolution}, p->refresh_rate});

            auto local_config = get_local_config_for(mir_output.output_id);

            DisplayConfigurationOutput const output{
                DisplayConfigurationOutputId(mir_output.output_id),
                DisplayConfigurationCardId(mir_output.card_id),
                DisplayConfigurationOutputType(mir_output.type),
                std::move(formats),
                std::move(modes),
                mir_output.preferred_mode,
                geometry::Size{mir_output.physical_width_mm, mir_output.physical_height_mm},
                !!mir_output.connected,
                !!mir_output.used,
                geometry::Point{mir_output.position_x, mir_output.position_y},
                mir_output.current_mode,
                mir_output.current_format,
                mir_output.power_mode,
                mir_output.orientation,
                local_config.scale,
                local_config.form_factor
            };

            f(output);
        });
}

void mgn::NestedDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput&)> f)
{
    // This is mostly copied and pasted from the const version above, but this
    // mutable version copies user-changes to the output structure at the end.

    std::for_each(
        display_config->outputs,
        display_config->outputs+display_config->num_outputs,
        [&f, this](MirDisplayOutput& mir_output)
        {
            std::vector<MirPixelFormat> formats;
            formats.reserve(mir_output.num_output_formats);
            for (auto p = mir_output.output_formats;
                 p != mir_output.output_formats+mir_output.num_output_formats;
                 ++p)
            {
                formats.push_back(*p);
            }

            std::vector<DisplayConfigurationMode> modes;
            modes.reserve(mir_output.num_modes);
            for (auto p = mir_output.modes;
                 p != mir_output.modes+mir_output.num_modes;
                 ++p)
            {
                modes.push_back(
                    DisplayConfigurationMode{
                        {p->horizontal_resolution, p->vertical_resolution},
                        p->refresh_rate});
            }

            auto local_config = get_local_config_for(mir_output.output_id);

            DisplayConfigurationOutput output{
                DisplayConfigurationOutputId(mir_output.output_id),
                DisplayConfigurationCardId(mir_output.card_id),
                DisplayConfigurationOutputType(mir_output.type),
                std::move(formats),
                std::move(modes),
                mir_output.preferred_mode,
                geometry::Size{mir_output.physical_width_mm,
                               mir_output.physical_height_mm},
                !!mir_output.connected,
                !!mir_output.used,
                geometry::Point{mir_output.position_x, mir_output.position_y},
                mir_output.current_mode,
                mir_output.current_format,
                mir_output.power_mode,
                mir_output.orientation,
                local_config.scale,
                local_config.form_factor
            };
            UserDisplayConfigurationOutput user(output);

            f(user);

            set_local_config_for(mir_output.output_id, {user.scale, user.form_factor });

            mir_output.current_mode = output.current_mode_index;
            mir_output.current_format = output.current_format;
            mir_output.position_x = output.top_left.x.as_int();
            mir_output.position_y = output.top_left.y.as_int();
            mir_output.used = output.used;
            mir_output.power_mode = output.power_mode;
            mir_output.orientation = output.orientation;
        });
}

std::unique_ptr<mg::DisplayConfiguration> mgn::NestedDisplayConfiguration::clone() const
{
    return std::make_unique<mgn::NestedDisplayConfiguration>(*this);
}

mgn::NestedDisplayConfiguration::LocalOutputConfig
mgn::NestedDisplayConfiguration::get_local_config_for(uint32_t output_id) const
{
    std::lock_guard<std::mutex> lock{local_config_mutex};

    constexpr LocalOutputConfig default_values {1.0f, mir_form_factor_monitor };

    bool inserted;
    decltype(local_config)::iterator keypair;

    std::tie(keypair, inserted) = local_config.insert(std::make_pair(output_id, default_values));

    // We don't care whether we inserted the default values or retrieved the previously set value.
    return keypair->second;
}

void mgn::NestedDisplayConfiguration::set_local_config_for(uint32_t output_id, LocalOutputConfig const& config)
{
    std::lock_guard<std::mutex> lock{local_config_mutex};

    local_config[output_id] = config;
}
