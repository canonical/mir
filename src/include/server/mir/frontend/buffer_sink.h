/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_FRONTEND_BUFFER_SINK_H_
#define MIR_FRONTEND_BUFFER_SINK_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/buffer_properties.h"
#include <string>

namespace mir
{
namespace graphics { class Buffer; }
namespace frontend
{
class BufferSink
{
public:
    virtual ~BufferSink() = default;

    virtual void send_buffer(frontend::BufferStreamId id, graphics::Buffer& buffer, graphics::BufferIpcMsgType) = 0;
    virtual void add_buffer(graphics::Buffer&) = 0;
    virtual void error_buffer(geometry::Size req_size, MirPixelFormat req_format, std::string const& error_msg) = 0;
    virtual void update_buffer(graphics::Buffer&) = 0;

protected:
    BufferSink() = default;
    BufferSink(BufferSink const&) = delete;
    BufferSink& operator=(BufferSink const&) = delete;
};
}
} // namespace mir

#endif // MIR_FRONTEND_BUFFER_SINK_H_
