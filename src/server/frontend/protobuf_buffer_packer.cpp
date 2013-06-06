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

namespace mfd=mir::frontend::detail;

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
