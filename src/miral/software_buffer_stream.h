/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_SOFTWARE_BUFFER_STREAM_H
#define MIRAL_SOFTWARE_BUFFER_STREAM_H

#include <mir/compositor/stream.h>
#include <mir/geometry/dimensions.h>
#include <mir/graphics/buffer.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/scene/surface.h>

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

namespace mir::graphics { class GraphicBufferAllocator; }

namespace miral
{

/// A mc::Stream that always reports having a submitted buffer so that
/// BasicSurface::generate_renderables() includes the surface in every frame.
class AlwaysHasSubmittedBufferStream : public mir::compositor::Stream
{
public:
    bool has_submitted_buffer() const override { return true; }
};

/// Wraps a buffer and calls a release callback when destroyed, returning it
/// to its owning BufferPool.
class ClaimedBuffer : public mir::graphics::Buffer
{
public:
    ClaimedBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer, std::function<void()>&& release_callback);

    ~ClaimedBuffer() override;

    auto map_readable() const -> std::unique_ptr<mir::renderer::software::Mapping<std::byte const>> override;
    mir::graphics::BufferID id() const override;
    mir::geometry::Size size() const override;
    MirPixelFormat pixel_format() const override;
    mir::graphics::NativeBufferBase* native_buffer_base() override;

private:
    std::shared_ptr<mir::graphics::Buffer> const buffer;
    std::function<void()> const release_callback;
};

/// Minimal double-buffered pool.  Callers claim one slot at a time via
/// claim(); the slot is returned to the pool when the ClaimedBuffer is
/// destroyed.  If all slots are in use the pool grows automatically.
class BufferPool
{
public:
    using BufferBuilder = std::function<std::shared_ptr<mir::graphics::Buffer>()>;

    BufferPool(BufferBuilder&& builder_func, size_t initial_size);

    /// Claims the next free buffer, growing the pool if necessary.
    std::shared_ptr<mir::graphics::Buffer> claim();

    /// Replaces the buffer builder and rebuilds all existing buffers.
    void set_builder(BufferBuilder&& builder);

private:
    struct Self
    {
        Self(BufferBuilder&& builder_func, size_t initial_size);
        std::mutex mutex;
        BufferBuilder builder;
        std::vector<std::pair<bool, std::shared_ptr<mir::graphics::Buffer>>> buffers;
    };

    std::shared_ptr<Self> data;
};

/// Creates a StreamInfo list containing a single AlwaysHasSubmittedBufferStream.
std::list<mir::scene::StreamInfo> create_stream_info();

} // namespace miral

#endif // MIRAL_SOFTWARE_BUFFER_STREAM_H
