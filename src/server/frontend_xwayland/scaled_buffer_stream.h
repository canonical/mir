/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_SCALED_BUFFER_STREAM_H
#define MIR_FRONTEND_SCALED_BUFFER_STREAM_H

#include "mir/compositor/buffer_stream.h"

namespace mir
{
namespace frontend
{
/// Wrappes another buffer stream and scales it's size, which is required for scaling XWayland surfaces without messing
/// with the scale of the buffer stream owned by the underlying WlSurface.
///
/// Note that even though shell->modify_surface() takes a frontend::BufferStream, this must implement
/// compositor::BufferStream as well because some dynamic casting happens somewhere.
class ScaledBufferStream : public compositor::BufferStream
{
public:
    ScaledBufferStream(std::shared_ptr<compositor::BufferStream>&& inner, float scale);

    /// Overrides from frontend::BufferStream
    /// @{
    void submit_buffer(std::shared_ptr<graphics::Buffer> const& buffer);
    void set_frame_posted_callback(std::function<void(geometry::Size const&)> const& callback);
    void with_most_recent_buffer_do(std::function<void(graphics::Buffer&)> const& exec);
    MirPixelFormat pixel_format() const;
    void set_scale(float scale);
    /// @}

    /// Overrides from compositor::BufferStream
    /// @{
    auto lock_compositor_buffer(void const* user_id) -> std::shared_ptr<graphics::Buffer>;
    auto stream_size() -> geometry::Size;
    auto buffers_ready_for_compositor(void const* user_id) const -> int;
    auto has_submitted_buffer() const -> bool;
    /// @}

private:
    std::shared_ptr<compositor::BufferStream> const inner;
    float const inv_scale;
};
}
}

#endif // MIR_FRONTEND_SCALED_BUFFER_STREAM_H
