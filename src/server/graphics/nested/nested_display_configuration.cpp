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

namespace mgn = mir::graphics::nested;

mgn::NestedDisplayConfiguration::NestedDisplayConfiguration(MirConnection* connection) :
display_config{connection}
{
}

mgn::NestedDisplayConfiguration::~NestedDisplayConfiguration() noexcept
{
}

void mgn::NestedDisplayConfiguration::for_each_card(std::function<void(DisplayConfigurationCard const&)>) const
{
    // TODO
}

void mgn::NestedDisplayConfiguration::for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const
{
    for (auto egl_display_info = display_config->outputs; egl_display_info != display_config->outputs+display_config->num_outputs; ++egl_display_info)
    {
        std::vector<geometry::PixelFormat> formats;
        formats.reserve(egl_display_info->num_output_formats);
        for (auto p = egl_display_info->output_formats; p != egl_display_info->output_formats+egl_display_info->num_output_formats; ++p)
            formats.push_back(geometry::PixelFormat(*p));

        std::vector<DisplayConfigurationMode> modes;
        modes.reserve(egl_display_info->num_modes);
        for (auto p = egl_display_info->modes; p != egl_display_info->modes+egl_display_info->num_modes; ++p)
            modes.push_back(DisplayConfigurationMode{{p->horizontal_resolution, p->vertical_resolution}, p->refresh_rate});

        DisplayConfigurationOutput const output{
            DisplayConfigurationOutputId(egl_display_info->output_id),
            DisplayConfigurationCardId(egl_display_info->card_id),
            DisplayConfigurationOutputType(egl_display_info->type),
            std::move(formats),
            std::move(modes),
            egl_display_info->preferred_mode,
            geometry::Size{egl_display_info->physical_width_mm, egl_display_info->physical_height_mm},
            !!egl_display_info->connected,
            !!egl_display_info->used,
            geometry::Point{egl_display_info->position_x, egl_display_info->position_y},
            egl_display_info->current_mode,
            egl_display_info->current_output_format
        };

        f(output);
    }
}

void mgn::NestedDisplayConfiguration::configure_output(DisplayConfigurationOutputId /*id*/, bool /*used*/, geometry::Point /*top_left*/, size_t /*mode_index*/)
{
    // TODO
}
