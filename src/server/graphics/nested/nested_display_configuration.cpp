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
#include "mir_toolkit/mir_display_configuration.h"
#include "mir_toolkit/mir_blob.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>


namespace mg = mir::graphics;
namespace mgn = mg::nested;

namespace
{
auto copy_config(MirDisplayConfig* conf) -> std::shared_ptr<MirDisplayConfig>
{
    auto const blob = mir::raii::deleter_for(
        mir_blob_from_display_config(conf),
        [] (MirBlob* b) { mir_blob_release(b); });

    return std::shared_ptr<MirDisplayConfig>{
        mir_blob_to_display_config(blob.get()),
        [] (MirDisplayConfig* c) { if (c) mir_display_config_release(c); }};
}
}

mgn::NestedDisplayConfiguration::NestedDisplayConfiguration(
    std::shared_ptr<MirDisplayConfig> const& display_config)
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
    std::function<void(DisplayConfigurationCard const&)> /*f*/) const
{
    // TODO Remove as the information is not known
}

mg::DisplayConfigurationOutput mgn::NestedDisplayConfiguration::create_display_output_config_from(MirOutput const* output) const
{
    size_t num_pixel_formats = mir_output_get_num_pixel_formats(output);
    std::vector<MirPixelFormat> formats;

    for (size_t n_pf = 0; n_pf < num_pixel_formats; n_pf++)
    {
        formats.push_back(mir_output_get_pixel_format(output, n_pf));
    }

    size_t num_modes = mir_output_get_num_modes(output);
    std::vector<DisplayConfigurationMode> modes;

    for (size_t n_m = 0; n_m < num_modes; n_m++)
    {
        auto mode         = mir_output_get_mode(output, n_m);
        auto width        = mir_output_mode_get_width(mode);
        auto height       = mir_output_mode_get_height(mode);
        auto refresh_rate = mir_output_mode_get_refresh_rate(mode);
        modes.push_back(DisplayConfigurationMode{{width, height}, refresh_rate});
    }

    auto output_id       = mir_output_get_id(output);
    auto output_type     = mir_output_get_type(output);
    auto width_mm        = mir_output_get_physical_width_mm(output);
    auto height_mm       = mir_output_get_physical_height_mm(output);
    auto connected       = mir_output_get_connection_state(output);
    auto used            = mir_output_is_enabled(output);
    auto position_x      = mir_output_get_position_x(output);
    auto position_y      = mir_output_get_position_y(output);
    auto current_format  = mir_output_get_current_pixel_format(output);
    auto power_mode      = mir_output_get_power_mode(output);
    auto orientation     = mir_output_get_orientation(output);
    auto local_config    = get_local_config_for(output);
    uint32_t preferred_index = mir_output_get_preferred_mode_index(output);
    uint32_t current_index   = mir_output_get_current_mode_index(output);

    std::vector<uint8_t> edid;
    auto edid_size = mir_output_get_edid_size(output);
    auto edid_start = mir_output_get_edid(output);
    if (edid_size && edid_start)
        edid.assign(edid_start, edid_start+edid_size);

    return mg::DisplayConfigurationOutput{
        DisplayConfigurationOutputId(output_id),
        DisplayConfigurationCardId(0), // Information not around
        DisplayConfigurationOutputType(output_type),
        std::move(formats),
        std::move(modes),
        preferred_index,
        geometry::Size{width_mm, height_mm},
        connected == mir_output_connection_state_connected,
        used,
        geometry::Point{position_x, position_y},
        current_index,
        current_format,
        power_mode,
        orientation,
        local_config.scale,
        local_config.form_factor,
        local_config.subpixel_arrangement,
        local_config.gamma,
        local_config.gamma_supported,
        std::move(edid)
    };
}

void mgn::NestedDisplayConfiguration::for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const
{
    size_t num_outputs = mir_display_config_get_num_outputs(display_config.get());
    for (size_t n_o = 0; n_o < num_outputs; n_o++)
    {
        auto mir_output = mir_display_config_get_output(display_config.get(), n_o);
        DisplayConfigurationOutput const output(create_display_output_config_from(mir_output));
        f(output);
    }
}

void mgn::NestedDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput&)> f)
{
    // This is mostly copied and pasted from the const version above, but this
    // mutable version copies user-changes to the output structure at the end.

    size_t num_outputs = mir_display_config_get_num_outputs(display_config.get());
    for (size_t n_o = 0; n_o < num_outputs; n_o++)
    {
        auto mir_output = mir_display_config_get_mutable_output(display_config.get(), n_o);
        DisplayConfigurationOutput output(create_display_output_config_from(mir_output));
        UserDisplayConfigurationOutput user(output);

        f(user);

        auto output_id = mir_output_get_id(mir_output);
        set_local_config_for(output_id,
            {user.scale, user.form_factor, user.subpixel_arrangement,
             user.gamma, user.gamma_supported});

        if (mir_output_get_num_modes(mir_output) > 0)
        {
            mir_output_set_current_mode(mir_output, mir_output_get_mode(mir_output, output.current_mode_index));
            mir_output_set_pixel_format(mir_output, output.current_format);
            mir_output_set_position(mir_output, output.top_left.x.as_int(), output.top_left.y.as_int());
            mir_output_set_power_mode(mir_output, output.power_mode);
            mir_output_set_orientation(mir_output, output.orientation);

            if (output.used)
            {
                mir_output_enable(mir_output);
            }
            else
            {
                mir_output_disable(mir_output);
            }
        }
        else
        {
            mir_output_disable(mir_output);
        }
    }
}

std::unique_ptr<mg::DisplayConfiguration> mgn::NestedDisplayConfiguration::clone() const
{
    return std::make_unique<mgn::NestedDisplayConfiguration>(*this);
}

mgn::NestedDisplayConfiguration::LocalOutputConfig
mgn::NestedDisplayConfiguration::get_local_config_for(MirOutput const* output) const
{
    std::lock_guard<std::mutex> lock{local_config_mutex};

    auto const output_id = mir_output_get_id(output);

    LocalOutputConfig const default_values{
        1.0f, mir_form_factor_monitor,
        mir_output_get_subpixel_arrangement(output),
        {}, mir_output_gamma_unsupported};

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
