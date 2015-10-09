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

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>


namespace mg = mir::graphics;
namespace mgn = mg::nested;

mgn::NestedDisplayConfiguration::NestedDisplayConfiguration(
    std::shared_ptr<MirDisplayConfiguration> const& display_config)
    : display_config{display_config}
{
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
        [&f](MirDisplayOutput const& mir_output)
        {
            std::vector<MirPixelFormat> formats;
            formats.reserve(mir_output.num_output_formats);
            for (auto p = mir_output.output_formats; p != mir_output.output_formats+mir_output.num_output_formats; ++p)
                formats.push_back(MirPixelFormat(*p));

            std::vector<DisplayConfigurationMode> modes;
            modes.reserve(mir_output.num_modes);
            for (auto p = mir_output.modes; p != mir_output.modes+mir_output.num_modes; ++p)
                modes.push_back(DisplayConfigurationMode{{p->horizontal_resolution, p->vertical_resolution}, p->refresh_rate});

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
                1.0f,
                mir_form_factor_monitor
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
        [&f](MirDisplayOutput& mir_output)
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
                1.0f,
                mir_form_factor_monitor
            };
            UserDisplayConfigurationOutput user(output);

            f(user);

            mir_output.current_mode = output.current_mode_index;
            mir_output.current_format = output.current_format;
            mir_output.position_x = output.top_left.x.as_int();
            mir_output.position_y = output.top_left.y.as_int();
            mir_output.used = output.used;
            mir_output.power_mode = output.power_mode;
            mir_output.orientation = output.orientation;
        });
}

