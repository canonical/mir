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
namespace mp = mir::protobuf;

namespace
{
mp::DisplayConfiguration* client_to_config(MirDisplayConfig* config)
{
    return reinterpret_cast<mp::DisplayConfiguration*>(config);
}

mp::DisplayConfiguration const* client_to_config(MirDisplayConfig const* config)
{
    return reinterpret_cast<mp::DisplayConfiguration const*>(config);
}

MirOutput* output_to_client(mp::DisplayOutput* output)
{
    return reinterpret_cast<MirOutput*>(output);
}

MirOutput const* output_to_client(mp::DisplayOutput const* output)
{
    return reinterpret_cast<MirOutput const*>(output);
}

mp::DisplayOutput* client_to_output(MirOutput* client)
{
    return reinterpret_cast<mp::DisplayOutput*>(client);
}

mp::DisplayOutput const* client_to_output(MirOutput const* client)
{
    return reinterpret_cast<mp::DisplayOutput const*>(client);
}

mp::DisplayMode const* client_to_mode(MirOutputMode const* client)
{
    return reinterpret_cast<mp::DisplayMode const*>(client);
}

MirOutputMode const* mode_to_client(mp::DisplayMode const* mode)
{
    return reinterpret_cast<MirOutputMode const*>(mode);
}
}

int mir_display_config_get_num_outputs(MirDisplayConfig const* client)
{
    auto config = client_to_config(client);
    return config->display_output_size();
}

MirOutput const* mir_display_config_get_output(MirDisplayConfig const* client_config, size_t index)
{
    auto config = client_to_config(client_config);

    mir::require(index < static_cast<size_t>(mir_display_config_get_num_outputs(client_config)));

    return output_to_client(&config->display_output(index));
}

MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* client_config, size_t index)
{
    auto config = client_to_config(client_config);

    mir::require(index < static_cast<size_t>(mir_display_config_get_num_outputs(client_config)));

    return output_to_client(config->mutable_display_output(index));
}

bool mir_output_is_enabled(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return (output->used() != 0);
}

void mir_output_enable(MirOutput* client_output)
{
    auto output = client_to_output(client_output);

    output->set_used(1);
}

void mir_output_disable(MirOutput* client_output)
{
    auto output = client_to_output(client_output);

    output->set_used(0);
}

int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* client_config)
{
    auto config = client_to_config(client_config);

    return config->display_card(0).max_simultaneous_outputs();
}

int mir_output_get_id(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->output_id();
}

MirOutputType mir_output_get_type(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return static_cast<MirOutputType>(output->type());
}

int mir_output_get_physical_width_mm(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    return output->physical_width_mm();
}

int mir_output_get_num_modes(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->mode_size();
}

MirOutputMode const* mir_output_get_preferred_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    if (output->preferred_mode() >= static_cast<size_t>(mir_output_get_num_modes(client_output)))
    {
        return nullptr;
    }
    else
    {
        return mode_to_client(&output->mode(output->preferred_mode()));
    }
}

MirOutputMode const* mir_output_get_current_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    if (output->current_mode() >= static_cast<size_t>(mir_output_get_num_modes(client_output)))
    {
        return nullptr;
    }
    else
    {
        return mode_to_client(&output->mode(output->current_mode()));
    }
}

void mir_output_set_current_mode(MirOutput* client_output, MirOutputMode const* client_mode)
{
    auto output = client_to_output(client_output);
    auto mode = client_to_mode(client_mode);

    int index = -1;
    for (int i = 0; i < output->mode_size(); ++i)
    {
        if (mode->SerializeAsString() == output->mode(i).SerializeAsString())
        {
            index = i;
            break;
        }
    }

    mir::require(index >= 0);
    mir::require(index < mir_output_get_num_modes(client_output));

    output->set_current_mode(static_cast<uint32_t>(index));
}

MirOutputMode const* mir_output_get_mode(MirOutput const* client_output, size_t index)
{
    auto output = client_to_output(client_output);

    return mode_to_client(&output->mode(index));
}

int mir_output_get_num_pixel_formats(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->pixel_format_size();
}

MirPixelFormat mir_output_get_pixel_format(MirOutput const* client_output, size_t index)
{
    auto output = client_to_output(client_output);

    return static_cast<MirPixelFormat>(output->pixel_format(index));
}

MirPixelFormat mir_output_get_current_pixel_format(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return static_cast<MirPixelFormat>(output->current_format());
}

void mir_output_set_pixel_format(MirOutput* client_output, MirPixelFormat format)
{
    auto output = client_to_output(client_output);

    // TODO: Maybe check format validity?
    output->set_current_format(format);
}

int mir_output_get_position_x(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->position_x();
}

int mir_output_get_position_y(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return output->position_y();
}

void mir_output_set_position(MirOutput* client_output, int x, int y)
{
    auto output = client_to_output(client_output);

    output->set_position_x(x);
    output->set_position_y(y);
}

MirOutputConnectionState mir_output_get_connection_state(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    // TODO: actually plumb through mir_output_connection_state_unknown.
    return output->connected() == 0 ? mir_output_connection_state_disconnected :
           mir_output_connection_state_connected;
}

int mir_output_get_physical_height_mm(MirOutput const *client_output)
{
    auto output = client_to_output(client_output);

    return output->physical_height_mm();
}

MirPowerMode mir_output_get_power_mode(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return static_cast<MirPowerMode>(output->power_mode());
}

void mir_output_set_power_mode(MirOutput* client_output, MirPowerMode mode)
{
    auto output = client_to_output(client_output);

    output->set_power_mode(mode);
}

MirOrientation mir_output_get_orientation(MirOutput const* client_output)
{
    auto output = client_to_output(client_output);

    return static_cast<MirOrientation>(output->orientation());
}

int mir_output_mode_get_width(MirOutputMode const* client_mode)
{
    auto mode = client_to_mode(client_mode);

    return mode->horizontal_resolution();
}

int mir_output_mode_get_height(MirOutputMode const* client_mode)
{
    auto mode = client_to_mode(client_mode);

    return mode->vertical_resolution();
}

double mir_output_mode_get_refresh_rate(MirOutputMode const* client_mode)
{
    auto mode = client_to_mode(client_mode);

    return mode->refresh_rate();
}
