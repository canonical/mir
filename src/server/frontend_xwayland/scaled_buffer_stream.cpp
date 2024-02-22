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

#include "scaled_buffer_stream.h"
#include "mir/geometry/forward.h"
#include "mir/geometry/rectangle.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::ScaledBufferStream::ScaledBufferStream(std::shared_ptr<compositor::BufferStream>&& inner, float scale)
    : inner{std::move(inner)},
      inv_scale{1.0f / scale}
{
}

void mf::ScaledBufferStream::submit_buffer(
    std::shared_ptr<graphics::Buffer> const& buffer,
    geom::Size dest_size,
    geom::RectangleD src_bounds)
{
    inner->submit_buffer(buffer, dest_size, src_bounds);
}

void mf::ScaledBufferStream::set_frame_posted_callback(std::function<void(geometry::Size const&)> const& callback)
{
    // Does this need to be scaled? I don't ? think ? so? compositor::Stream seems to leave it unscaled.
    inner->set_frame_posted_callback(callback);
}

MirPixelFormat mf::ScaledBufferStream::pixel_format() const
{
    return inner->pixel_format();
}

void mf::ScaledBufferStream::set_scale(float scale)
{
    // Pass on the given scale to the inner stream, this is unrelated from the scale we apply.
    inner->set_scale(scale);
}

auto mf::ScaledBufferStream::lock_compositor_buffer(void const* user_id) -> std::shared_ptr<graphics::Buffer>
{
    return inner->lock_compositor_buffer(user_id);
}

auto mf::ScaledBufferStream::stream_size() -> geometry::Size
{
    // This is it. This is what the whole class is for.
    return inner->stream_size() * inv_scale;
}

auto mf::ScaledBufferStream::has_submitted_buffer() const -> bool
{
    return inner->has_submitted_buffer();
}

