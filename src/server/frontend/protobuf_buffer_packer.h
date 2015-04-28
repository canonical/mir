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

#include "mir/graphics/buffer_ipc_message.h"
#include <memory>

namespace mir
{
namespace protobuf
{
class Buffer;
class DisplayConfiguration;
}

namespace graphics
{
class DisplayConfiguration;
}

namespace frontend
{
namespace detail
{

void pack_protobuf_display_configuration(protobuf::DisplayConfiguration& protobuf_config,
                                         graphics::DisplayConfiguration const& display_config);

class ProtobufBufferPacker : public graphics::BufferIpcMessage
{
public:
    ProtobufBufferPacker(protobuf::Buffer*);
    void pack_fd(Fd const&);
    void pack_data(int);
    void pack_stride(geometry::Stride);
    void pack_flags(unsigned int);
    void pack_size(geometry::Size const& size);

    std::vector<Fd> fds();
    std::vector<int> data();
private:
    std::vector<mir::Fd> fds_;
    protobuf::Buffer* buffer_response;
};

}
}
}

#endif /* MIR_FRONTEND_PROTOBUF_BUFFER_PACKER_H_ */
