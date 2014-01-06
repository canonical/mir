/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_STREAM_SCENE_H_
#define MIR_COMPOSITOR_BUFFER_STREAM_SCENE_H_

#include "mir/compositor/buffer_stream.h"

#include <mutex>

namespace mir
{
namespace compositor
{

class BufferIDUniqueGenerator;
class BufferBundle;
class BackBufferStrategy;

class BufferStreamSurfaces : public BufferStream
{
public:
    BufferStreamSurfaces(std::shared_ptr<BufferBundle> const& swapper);
    ~BufferStreamSurfaces();

    void swap_client_buffers(graphics::Buffer*& buffer) override;

    std::shared_ptr<graphics::Buffer>
        lock_compositor_buffer(unsigned long frameno) override;
    std::shared_ptr<graphics::Buffer> lock_snapshot_buffer() override;

    MirPixelFormat get_stream_pixel_format() override;
    geometry::Size stream_size() override;
    void resize(geometry::Size const& size) override;
    void allow_framedropping(bool) override;
    void force_requests_to_complete() override;

protected:
    BufferStreamSurfaces(const BufferStreamSurfaces&) = delete;
    BufferStreamSurfaces& operator=(const BufferStreamSurfaces&) = delete;

private:
    std::shared_ptr<BufferBundle> const buffer_bundle;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_STREAM_SCENE_H_ */
