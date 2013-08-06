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

#ifndef MIR_FRONTEND_PROTOBUF_BUFFER_PACKER_H_
#define MIR_FRONTEND_PROTOBUF_BUFFER_PACKER_H_

#include "mir/graphics/buffer_ipc_packer.h"
#include "mir_protobuf.pb.h"

namespace mir
{
namespace graphics
{
struct DisplayConfigurationOutput;
}
namespace frontend
{
namespace detail
{

void pack_protobuf_display_output(protobuf::DisplayOutput* output, graphics::DisplayConfigurationOutput const&);

class ProtobufBufferPacker : public graphics::BufferIPCPacker
{
public:
    ProtobufBufferPacker(protobuf::Buffer*);
    void pack_fd(int);
    void pack_data(int);
    void pack_stride(geometry::Stride);
private:
    protobuf::Buffer* buffer_response;
};

}
}
}

#endif /* MIR_FRONTEND_PROTOBUF_BUFFER_PACKER_H_ */
