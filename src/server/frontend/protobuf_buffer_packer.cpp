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

namespace mfd=mir::frontend::detail;

void mfd::pack_protobuf_display_output(protobuf::DisplayOutput* output,
                                       graphics::DisplayConfigurationOutput const& config)
{
    output->set_output_id(config.id.as_value());
    output->set_card_id(config.card_id.as_value());
    output->set_connected(config.connected);
    output->set_used(config.used);
    output->set_physical_width_mm(config.physical_size_mm.width.as_uint32_t());
    output->set_physical_height_mm(config.physical_size_mm.height.as_uint32_t());
    output->set_position_x(config.top_left.x.as_uint32_t());
    output->set_position_y(config.top_left.y.as_uint32_t());
    for (auto const& mode : config.modes)
    {
        auto output_mode = output->add_mode();
        output_mode->set_horizontal_resolution(mode.size.width.as_uint32_t()); 
        output_mode->set_vertical_resolution(mode.size.height.as_uint32_t());
        output_mode->set_refresh_rate(mode.vrefresh_hz);
    }
    output->set_current_mode(config.current_mode_index);

    for (auto const& pf : config.pixel_formats)
    {
        output->add_pixel_format(static_cast<uint32_t>(pf));
    }
    output->set_current_format(config.current_format_index);
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
