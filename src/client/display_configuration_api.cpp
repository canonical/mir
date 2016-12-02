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
#include "mir/output_type_names.h"
#include "display_configuration.h"
#include "mir/uncaught.h"
#include <cstring>

namespace mcl = mir::client;
namespace mp = mir::protobuf;

// We never create this type, just use pointers to it to opaquify mp::DisplayOutput
struct MirOutput : mp::DisplayOutput { MirOutput() = delete; };

namespace
{

MirOutput* output_to_client(mp::DisplayOutput* output)
{
    return static_cast<MirOutput*>(output);
}

MirOutput const* output_to_client(mp::DisplayOutput const* output)
{
    return static_cast<MirOutput const*>(output);
}

mp::DisplayMode const* client_to_mode(MirOutputMode const* client)
{
    return reinterpret_cast<mp::DisplayMode const*>(client);
}

MirOutputMode const* mode_to_client(mp::DisplayMode const* mode)
{
    return reinterpret_cast<MirOutputMode const*>(mode);
}

/*
 * This EDID code could move up to the server later, but that's not required
 * yet. Maybe when we want to dump it to the server log?
 */
size_t edid_get_descriptor_string(uint8_t const* edid, uint8_t type,
                                  char str[14])
{
    union descriptor
    {
        struct
        {
            uint16_t pixel_clock;
        } detailed_timing;
        struct
        {
            uint16_t zero0;
            uint8_t  zero2;
            uint8_t  type;
            uint8_t  zero4;
            char     text[13];
        } other;
    };
    
    union descriptor const* desc = (union descriptor const*)(edid + 54);
    union descriptor const* desc_end = desc + 4;
    size_t len = 0;

    for (; desc < desc_end; ++desc)
    {
        if (!desc->detailed_timing.pixel_clock && desc->other.type == type)
        {
            len = 13;
            memcpy(str, desc->other.text, len);
            break;
        }
    }
    str[len] = '\0';
    return len;
}

size_t edid_get_monitor_name(uint8_t const* edid, char str[14])
{
    size_t len = edid_get_descriptor_string(edid, 0xFC, str);
    if (char* pad = strchr(str, '\n'))
    {
        *pad = '\0';
        len = pad - str;
    }
    return len;
}

void edid_get_manufacturer(uint8_t const* edid, char str[4])
{
    uint16_t manufacturer = static_cast<uint16_t>(edid[8]) << 8 | edid[9];
    str[0] = ((manufacturer >> 10) & 31) + 'A' - 1;
    str[1] = ((manufacturer >> 5) & 31) + 'A' - 1;
    str[2] = (manufacturer & 31) + 'A' - 1;
    str[3] = '\0';
}

uint16_t edid_get_product_code(uint8_t const* edid)
{
    return static_cast<uint16_t>(edid[11]) << 8 | edid[10];
}

} // namespace

int mir_display_config_get_num_outputs(MirDisplayConfig const* config)
{
    return config->display_output_size();
}

MirOutput const* mir_display_config_get_output(MirDisplayConfig const* config, size_t index)
{
    mir::require(index < static_cast<size_t>(mir_display_config_get_num_outputs(config)));

    return output_to_client(&config->display_output(index));
}

MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* config, size_t index)
{
    mir::require(index < static_cast<size_t>(mir_display_config_get_num_outputs(config)));

    return output_to_client(config->mutable_display_output(index));
}

bool mir_output_is_enabled(MirOutput const* output)
{
    return (output->used() != 0);
}

void mir_output_enable(MirOutput* output)
{
    output->set_used(1);
}

void mir_output_disable(MirOutput* output)
{
    output->set_used(0);
}

char const* mir_output_get_model(MirOutput const* output)
{
    // In future this might be provided by the server itself...
    if (output->has_model())
        return output->model().c_str();

    // But if not we use the same member for caching our EDID probe...
    if (auto edid = mir_output_get_edid(output))
    {
        char name[14];
        if (!edid_get_monitor_name(edid, name))
        {
            edid_get_manufacturer(edid, name);
            snprintf(name+3, 11, " %hu", edid_get_product_code(edid));
        }
        const_cast<MirOutput*>(output)->set_model(name);
        return output->model().c_str();
    }

    return nullptr;
}

int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* config)
{
    return config->display_card(0).max_simultaneous_outputs();
}

int mir_output_get_id(MirOutput const* output)
{
    return output->output_id();
}

MirOutputType mir_output_get_type(MirOutput const* output)
{
    return static_cast<MirOutputType>(output->type());
}

char const* mir_display_output_type_name(MirDisplayOutputType type)
{
    return mir::output_type_name(type);
}

char const* mir_output_type_name(MirOutputType type)
{
    return mir::output_type_name(type);
}

int mir_output_get_physical_width_mm(MirOutput const *output)
{
    return output->physical_width_mm();
}

int mir_output_get_num_modes(MirOutput const* output)
{
    return output->mode_size();
}

MirOutputMode const* mir_output_get_preferred_mode(MirOutput const* output)
{
    if (output->preferred_mode() >= static_cast<size_t>(mir_output_get_num_modes(output)))
    {
        return nullptr;
    }
    else
    {
        return mode_to_client(&output->mode(output->preferred_mode()));
    }
}

size_t mir_output_get_preferred_mode_index(MirOutput const* output)
{
    if (output->preferred_mode() >= static_cast<size_t>(mir_output_get_num_modes(output)))
    {
        return std::numeric_limits<size_t>::max();
    }
    else
    {
        return output->preferred_mode();
    }
}

MirOutputMode const* mir_output_get_current_mode(MirOutput const* output)
{
    if (output->current_mode() >= static_cast<size_t>(mir_output_get_num_modes(output)))
    {
        return nullptr;
    }
    else
    {
        return mode_to_client(&output->mode(output->current_mode()));
    }
}

size_t mir_output_get_current_mode_index(MirOutput const* output)
{
    if (output->current_mode() >= static_cast<size_t>(mir_output_get_num_modes(output)))
    {
        return std::numeric_limits<size_t>::max();
    }
    else
    {
        return output->current_mode();
    }
}


void mir_output_set_current_mode(MirOutput* output, MirOutputMode const* client_mode)
{
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
    mir::require(index < mir_output_get_num_modes(output));

    output->set_current_mode(static_cast<uint32_t>(index));
}

MirOutputMode const* mir_output_get_mode(MirOutput const* output, size_t index)
{
    return mode_to_client(&output->mode(index));
}

int mir_output_get_num_pixel_formats(MirOutput const* output)
{
    return output->pixel_format_size();
}

MirPixelFormat mir_output_get_pixel_format(MirOutput const* output, size_t index)
{
    return static_cast<MirPixelFormat>(output->pixel_format(index));
}

MirPixelFormat mir_output_get_current_pixel_format(MirOutput const* output)
{
    return static_cast<MirPixelFormat>(output->current_format());
}

void mir_output_set_pixel_format(MirOutput* output, MirPixelFormat format)
{
    // TODO: Maybe check format validity?
    output->set_current_format(format);
}

int mir_output_get_position_x(MirOutput const* output)
{
    return output->position_x();
}

int mir_output_get_position_y(MirOutput const* output)
{
    return output->position_y();
}

void mir_output_set_position(MirOutput* output, int x, int y)
{
    output->set_position_x(x);
    output->set_position_y(y);
}

MirOutputConnectionState mir_output_get_connection_state(MirOutput const *output)
{
    // TODO: actually plumb through mir_output_connection_state_unknown.
    return output->connected() == 0 ? mir_output_connection_state_disconnected :
           mir_output_connection_state_connected;
}

int mir_output_get_physical_height_mm(MirOutput const *output)
{
    return output->physical_height_mm();
}

MirPowerMode mir_output_get_power_mode(MirOutput const* output)
{
    return static_cast<MirPowerMode>(output->power_mode());
}

void mir_output_set_power_mode(MirOutput* output, MirPowerMode mode)
{
    output->set_power_mode(mode);
}

MirOrientation mir_output_get_orientation(MirOutput const* output)
{
    return static_cast<MirOrientation>(output->orientation());
}

void mir_output_set_orientation(MirOutput* output, MirOrientation orientation)
{
    output->set_orientation(orientation);
}


float mir_output_get_scale_factor(MirOutput const* output)
{
    return output->scale_factor();
}

MirSubpixelArrangement mir_output_get_subpixel_arrangement(MirOutput const* output)
{
    return static_cast<MirSubpixelArrangement>(output->subpixel_arrangement());
}

uint32_t mir_output_get_gamma_size(MirOutput const* output)
{
    return (output->gamma_red().size() / (sizeof(uint16_t) / sizeof(char)));
}

bool mir_output_is_gamma_supported(MirOutput const* output)
{
    return output->gamma_supported();
}

void mir_output_get_gamma(MirOutput const* output,
                          uint16_t* red,
                          uint16_t* green,
                          uint16_t* blue,
                          uint32_t  size)
try
{
    auto red_bytes = output->gamma_red();
    auto green_bytes = output->gamma_green();
    auto blue_bytes = output->gamma_blue();

    // Check our number of bytes is equal to our uint16_t size
    mir::require(red_bytes.size() / (sizeof(uint16_t) / sizeof(char)) == size);

    std::copy(std::begin(red_bytes), std::end(red_bytes),
        reinterpret_cast<char*>(red));
    std::copy(std::begin(green_bytes), std::end(green_bytes),
        reinterpret_cast<char*>(green));
    std::copy(std::begin(blue_bytes), std::end(blue_bytes),
        reinterpret_cast<char*>(blue));

} catch (std::exception const& e) {
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

void mir_output_set_gamma(MirOutput* output,
                          uint16_t const* red,
                          uint16_t const* green,
                          uint16_t const* blue,
                          uint32_t  size)
try
{
    // Since we are going from a uint16_t to a char (int8_t) we are doubling the size
    output->set_gamma_red(reinterpret_cast<char const*>(red),
        size * (sizeof(uint16_t) / sizeof(char)));
    output->set_gamma_green(reinterpret_cast<char const*>(green),
        size * (sizeof(uint16_t) / sizeof(char)));
    output->set_gamma_blue(reinterpret_cast<char const*>(blue),
        size * (sizeof(uint16_t) / sizeof(char)));

    mir::require(size == mir_output_get_gamma_size(output));
} catch (std::exception const& e) {
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

MirFormFactor mir_output_get_form_factor(MirOutput const* output)
{
    return static_cast<MirFormFactor>(output->form_factor());
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

uint8_t const* mir_output_get_edid(MirOutput const* output)
{
    if (output->has_edid())
    {
        return reinterpret_cast<uint8_t const*>(output->edid().data());
    }
    return nullptr;
}

size_t mir_output_get_edid_size(MirOutput const* output)
{
    if (output->has_edid())
    {
        return output->edid().length();
    }
    return 0;
}
