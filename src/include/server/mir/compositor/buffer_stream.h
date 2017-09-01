/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
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

    virtual std::shared_ptr<graphics::Buffer>
        lock_compositor_buffer(void const* user_id) = 0;
    virtual geometry::Size stream_size() = 0;
    virtual int buffers_ready_for_compositor(void const* user_id) const = 0;
    virtual void drop_old_buffers() = 0;
    virtual bool has_submitted_buffer() const = 0;
    virtual bool framedropping() const = 0;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_STREAM_H_ */
