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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "protobuf_buffer_packer.h"

#include "mir/graphics/display_configuration.h"
#include "mir_protobuf.pb.h"

namespace mfd = mir::frontend::detail;
namespace mg = mir::graphics;
namespace mp = mir::protobuf;

namespace
{

void pack_protobuf_display_card(mp::DisplayCard& protobuf_card,
                                mg::DisplayConfigurationCard const& display_card)
{
    protobuf_card.set_card_id(display_card.id.as_value());
    protobuf_card.set_max_simultaneous_outputs(display_card.max_simultaneous_outputs);
}

void pack_protobuf_display_output(mp::DisplayOutput& protobuf_output,
                                  mg::DisplayConfigurationOutput const& display_output)
{
    protobuf_output.set_output_id(display_output.id.as_value());
    protobuf_output.set_card_id(display_output.card_id.as_value());
    protobuf_output.set_type(static_cast<uint32_t>(display_output.type));

    for (auto const& pf : display_output.pixel_formats)
    {
        protobuf_output.add_pixel_format(static_cast<uint32_t>(pf));
    }

    for (auto const& mode : display_output.modes)
    {
        auto protobuf_output_mode = protobuf_output.add_mode();
        protobuf_output_mode->set_horizontal_resolution(mode.size.width.as_uint32_t());
        protobuf_output_mode->set_vertical_resolution(mode.size.height.as_uint32_t());
        protobuf_output_mode->set_refresh_rate(mode.vrefresh_hz);
    }
    protobuf_output.set_preferred_mode(display_output.preferred_mode_index);

    protobuf_output.set_physical_width_mm(display_output.physical_size_mm.width.as_uint32_t());
    protobuf_output.set_physical_height_mm(display_output.physical_size_mm.height.as_uint32_t());

    protobuf_output.set_connected(display_output.connected);
    protobuf_output.set_used(display_output.used);
    protobuf_output.set_position_x(display_output.top_left.x.as_uint32_t());
    protobuf_output.set_position_y(display_output.top_left.y.as_uint32_t());
    protobuf_output.set_current_mode(display_output.current_mode_index);
    protobuf_output.set_current_format(static_cast<uint32_t>(display_output.current_format));
    protobuf_output.set_power_mode(static_cast<uint32_t>(display_output.power_mode));
    protobuf_output.set_orientation(display_output.orientation);
}

}

void mfd::pack_protobuf_display_configuration(mp::DisplayConfiguration& protobuf_config,
                                              mg::DisplayConfiguration const& display_config)
{
    display_config.for_each_card(
        [&protobuf_config](mg::DisplayConfigurationCard const& card)
        {
            auto protobuf_card = protobuf_config.add_display_card();
            pack_protobuf_display_card(*protobuf_card, card);
        });

    display_config.for_each_output(
        [&protobuf_config](mg::DisplayConfigurationOutput const& output)
        {
            auto protobuf_output = protobuf_config.add_display_output();
            pack_protobuf_display_output(*protobuf_output, output);
        });
}

mfd::ProtobufBufferPacker::ProtobufBufferPacker(protobuf::Buffer* response)
    : buffer_response(response)
{
}

void mfd::ProtobufBufferPacker::pack_fd(int fd)
{
    buffer_response->add_fd(fd);
}

void mfd::ProtobufBufferPacker::pack_data(int data)
{
    buffer_response->add_data(data);
}

void mfd::ProtobufBufferPacker::pack_stride(geometry::Stride stride)
{
    buffer_response->set_stride(stride.as_uint32_t());
}

void mfd::ProtobufBufferPacker::pack_flags(unsigned int flags)
{
    buffer_response->set_flags(flags);
}

void mfd::ProtobufBufferPacker::pack_size(geometry::Size const& size)
{
    buffer_response->set_width(size.width.as_int());
    buffer_response->set_height(size.height.as_int());
}

