/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <mir_toolkit/mir_display_configuration.h>
#include <mir/require.h>

#include "display_configuration.h"

namespace mcl = mir::client;

namespace
{
std::shared_ptr<mcl::DisplayConfiguration::Config>& client_to_config(MirDisplayConfig* config) __attribute__((pure));
std::shared_ptr<mcl::DisplayConfiguration::Config const>& client_to_config(MirDisplayConfig const* config) __attribute__((pure));

MirOutput* output_to_client(MirDisplayOutput* output) __attribute__((pure));
MirOutput const* output_to_client(MirDisplayOutput const* output) __attribute__((pure));
MirDisplayOutput* client_to_output(MirOutput* client) __attribute__((pure));
MirDisplayOutput const* client_to_output(MirOutput const* client) __attribute__((pure));


std::shared_ptr<mcl::DisplayConfiguration::Config>& client_to_config(MirDisplayConfig* config)
{
    return *reinterpret_cast<std::shared_ptr<mcl::DisplayConfiguration::Config>*>(config);
}

std::shared_ptr<mcl::DisplayConfiguration::Config const>& client_to_config(MirDisplayConfig const* config)
{
    return *reinterpret_cast<std::shared_ptr<mcl::DisplayConfiguration::Config const>*>(const_cast<MirDisplayConfig*>(config));
}

MirOutput* output_to_client(MirDisplayOutput* output)
{
    return reinterpret_cast<MirOutput*>(output);
}

MirOutput const* output_to_client(MirDisplayOutput const* output)
{
    return reinterpret_cast<MirOutput const*>(output);
}

MirDisplayOutput* client_to_output(MirOutput* client)
{
    return reinterpret_cast<MirDisplayOutput*>(client);
}

MirDisplayOutput const* client_to_output(MirOutput const* client)
{
    return reinterpret_cast<MirDisplayOutput const*>(client);
}
}

int mir_display_config_get_num_outputs(MirDisplayConfig const* client)
{
    auto config = client_to_config(client);
    return config->outputs.size();
}

MirOutput const* mir_display_config_get_output(MirDisplayConfig const* client_config, size_t index)
{
    auto config = client_to_config(client_config);

    mir::require(index < config->outputs.size());

    return output_to_client(&config->outputs[index]);
}

MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* client_config, size_t index)
{
    auto config = client_to_config(client_config);

    mir::require(index < config->outputs.size());

    return output_to_client(&config->outputs[index]);
}

bool mir_output_is_enabled(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return (output->used != 0);
}

void mir_output_enable(MirOutput* client_output)
{
    auto output = client_to_output(client_output);

    output->used = 1;
}

void mir_output_disable(MirOutput* client_output)
{
    auto output = client_to_output(client_output);

    output->used = 0;
}

int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* client_config)
{
    auto config = client_to_config(client_config);

    return config->cards[0].max_simultaneous_outputs;
}

int mir_output_get_id(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->output_id;
}

MirDisplayOutputType mir_output_get_type(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->type;
}

int mir_output_get_physical_width_mm(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    return output->physical_width_mm;
}

int mir_output_get_num_modes(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->num_modes;
}

MirOutputMode const* mir_output_get_preferred_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    if (output->preferred_mode >= output->num_modes)
    {
        return nullptr;
    }
    else
    {
        return reinterpret_cast<MirOutputMode const*>(&output->modes[output->preferred_mode]);
    }
}

MirOutputMode const* mir_output_get_current_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    if (output->current_mode >= output->num_modes)
    {
        return nullptr;
    }
    else
    {
        return reinterpret_cast<MirOutputMode const*>(&output->modes[output->current_mode]);
    }
}

void mir_output_set_current_mode(MirOutput* client_output, MirOutputMode const* mode)
{
    auto output = client_to_output(client_output);

    auto internal_mode = reinterpret_cast<MirDisplayMode const*>(mode);
    ptrdiff_t offset = internal_mode - output->modes;
    int index = offset / sizeof(MirDisplayMode);

    mir::require(index >= 0);
    mir::require(index < static_cast<int>(output->num_modes));

    output->current_mode = static_cast<uint32_t>(index);
}

MirOutputMode const* mir_output_get_mode(MirOutput const* client_output, size_t index)
{
    auto output = client_to_output(client_output);

    return reinterpret_cast<MirOutputMode const*>(&output->modes[index]);
}

int mir_output_get_num_output_formats(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->num_output_formats;
}

MirPixelFormat mir_output_get_format(MirOutput const* client_output, size_t index)
{
    auto output = client_to_output(client_output);

    return output->output_formats[index];
}

MirPixelFormat mir_output_get_current_format(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->current_format;
}

void mir_output_set_format(MirOutput* client_output, MirPixelFormat format)
{
    auto output = client_to_output(client_output);

    // TODO: Maybe check format validity?
    output->current_format = format;
}

int mir_output_get_position_x(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->position_x;
}

int mir_output_get_position_y(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->position_y;
}

MirOutputConnection mir_output_get_connection_state(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    // TODO: actually plumb through mir_output_connection_unknown.
    return output->connected == 0 ? mir_output_disconnected : mir_output_connected;
}

int mir_output_get_physical_height_mm(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    return output->physical_height_mm;
}

MirPowerMode mir_output_get_power_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->power_mode;
}

void mir_output_set_power_mode(MirOutput* client_output, MirPowerMode mode)
{
    auto output = client_to_output(client_output);

    output->power_mode = mode;
}

MirOrientation mir_output_get_orientation(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->orientation;
}

void mir_output_set_orientation(MirOutput* client_output, MirOrientation orientation)
{
    auto output = client_to_output(client_output);

    output->orientation = orientation;
}

int mir_output_mode_get_width(MirOutputMode const* mode)
{
    auto internal_mode = reinterpret_cast<MirDisplayMode const *>(mode);

    return internal_mode->horizontal_resolution;
}

int mir_output_mode_get_height(MirOutputMode const* mode)
{
    auto internal_mode = reinterpret_cast<MirDisplayMode const *>(mode);

    return internal_mode->vertical_resolution;
}

double mir_output_mode_get_refresh_rate(MirOutputMode const* mode)
{
    auto internal_mode = reinterpret_cast<MirDisplayMode const *>(mode);

    return internal_mode->refresh_rate;
}
