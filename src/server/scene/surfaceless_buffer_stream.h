/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SERVER_SCENE_SURFACELESS_BUFFER_STREAM_H_
#define MIR_SERVER_SCENE_SURFACELESS_BUFFER_STREAM_H_

#include "mir/frontend/buffer_stream.h"

namespace mir
{
namespace compositor
{
class BufferStream;
}
namespace scene
{
    
class SurfacelessBufferStream : public frontend::BufferStream
{
public:
    SurfacelessBufferStream(std::shared_ptr<compositor::BufferStream> const& buffer_stream);
    ~SurfacelessBufferStream() = default;

    void swap_buffers(graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete) override;
    
    void with_most_recent_buffer_do(std::function<void(graphics::Buffer&)> const& exec) override;
    
    void add_observer(std::shared_ptr<scene::SurfaceObserver> const& new_observer) override;
    
    void remove_observer(std::weak_ptr<scene::SurfaceObserver> const& /* observer */) override;

    MirPixelFormat pixel_format() const override;
    
private:
    std::shared_ptr<compositor::BufferStream> const buffer_stream;

    std::shared_ptr<scene::SurfaceObserver> observer;

    std::shared_ptr<graphics::Buffer> comp_buffer;
};
}
}

#endif // MIR_SERVER_SCENE_SURFACELESS_BUFFER_STREAM_H_
