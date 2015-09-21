/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_FRONTEND_CLIENT_BUFFERS_H_
#define MIR_FRONTEND_CLIENT_BUFFERS_H_

#include "mir/graphics/buffer_id.h"
#include <memory>

namespace mir
{
namespace graphics { class Buffer; class BufferProperties; }
namespace frontend
{
class ClientBuffers
{
public:
    virtual graphics::BufferID add_buffer(graphics::BufferProperties const& properties) = 0;
    virtual void remove_buffer(graphics::BufferID id) = 0;
    virtual std::shared_ptr<graphics::Buffer>& operator[](graphics::BufferID) = 0;
    virtual void send_buffer(graphics::BufferID id) = 0;

    ClientBuffers(ClientBuffers const&) = delete;
    ClientBuffers& operator=(ClientBuffers const&) = delete;
    virtual ~ClientBuffers() = default;
    ClientBuffers() = default;
};
}
}
#endif /* MIR_FRONTEND_CLIENT_BUFFERS_H_ */
