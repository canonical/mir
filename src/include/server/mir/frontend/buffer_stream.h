/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_BUFFER_STREAM_H_
#define MIR_FRONTEND_BUFFER_STREAM_H_

#include <mir_toolkit/common.h>
#include "mir/geometry/size.h"
#include <functional>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
struct BufferProperties;
}

namespace frontend
{

class BufferStream
{
public:
    virtual ~BufferStream() = default;

    virtual void submit_buffer(
        std::shared_ptr<graphics::Buffer> const& buffer,
        geometry::Size dest_size,
        geometry::RectangleD src_bounds) = 0;

    virtual void set_frame_posted_callback(
        std::function<void(geometry::Size const&)> const& callback) = 0;
protected:
    BufferStream() = default;
    BufferStream(BufferStream const&) = delete;
    BufferStream& operator=(BufferStream const&) = delete;
};

}
}


#endif /* MIR_FRONTEND_BUFFER_STREAM_H_ */
