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

#ifndef MIR_COMPOSITOR_BUFFER_STREAM_H_
#define MIR_COMPOSITOR_BUFFER_STREAM_H_

#include "mir/geometry/size.h"
#include "mir/frontend/buffer_stream.h"
#include "mir_toolkit/common.h"
#include "mir/graphics/buffer_id.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
}

namespace compositor
{

class BufferStream : public frontend::BufferStream
{
public:
    virtual ~BufferStream() = default;

    virtual auto lock_compositor_buffer(void const* user_id) -> std::shared_ptr<graphics::Buffer> = 0;
    /// Logical size of the stream (may be different than buffer sizes if scaled)
    virtual auto stream_size() -> geometry::Size = 0;
    virtual auto has_submitted_buffer() const -> bool = 0;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_STREAM_H_ */
