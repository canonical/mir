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

#include "software_buffer_pool.h"
#include "mir/fatal.h"

#include <mir/compositor/stream.h>
#include <mir/geometry/displacement.h>
#include <mir/graphics/buffer.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/report_exception.h>

namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;
namespace ms = mir::scene;

namespace
{
/// Wraps a buffer and calls a release callback when destroyed, returning it
/// to its owning SoftwareBufferPool.
class ClaimedBuffer : public mir::graphics::Buffer
{
public:
    ClaimedBuffer(std::shared_ptr<mir::graphics::Buffer> const& buffer, std::function<void()>&& release_callback) :
        buffer(buffer),
        release_callback(std::move(release_callback))
    {}

    ~ClaimedBuffer() override
    {
        try
        {
            release_callback();
        }
        catch (std::exception const& e)
        {
            MIR_FATAL_ERROR("Exception thrown while releasing a buffer back to its pool: {}", e.what());
        }
    }

    ClaimedBuffer(ClaimedBuffer const&) = delete;
    ClaimedBuffer& operator=(ClaimedBuffer const&) = delete;
    ClaimedBuffer(ClaimedBuffer&&) = delete;
    ClaimedBuffer& operator=(ClaimedBuffer&&) = delete;

    std::unique_ptr<mir::renderer::software::Mapping<std::byte const>> map_readable() const override
    { return buffer->map_readable(); }

    mir::graphics::BufferID id() const override { return buffer->id(); }
    mir::geometry::Size size() const override { return buffer->size(); }
    MirPixelFormat pixel_format() const override { return buffer->pixel_format(); }
    mir::graphics::NativeBufferBase* native_buffer_base() override { return buffer->native_buffer_base(); }

private:
    std::shared_ptr<mir::graphics::Buffer> const buffer;
    std::function<void()> const release_callback;
};
}

struct miral::SoftwareBufferPool::Self
{
    Self(BufferBuilder&& builder_func, size_t initial_size);
    std::mutex mutex;
    BufferBuilder builder;
    std::vector<std::pair<bool, std::shared_ptr<mir::graphics::Buffer>>> buffers;
};

miral::SoftwareBufferPool::Self::Self(BufferBuilder&& builder_func, size_t initial_size) : builder(std::move(builder_func))
{
    for (size_t i = 0; i < initial_size; ++i)
        buffers.emplace_back(false, builder());
}

miral::SoftwareBufferPool::SoftwareBufferPool(BufferBuilder&& builder_func, size_t initial_size) :
    data(std::make_shared<Self>(std::move(builder_func), initial_size))
{}

std::shared_ptr<mg::Buffer> miral::SoftwareBufferPool::claim()
{
    std::lock_guard lock{data->mutex};
    for (size_t i = 0; i < data->buffers.size(); ++i)
    {
        if (auto& [used, buffer] = data->buffers[i]; !used)
        {
            used = true;
            std::weak_ptr weak_data = data;
            return std::make_shared<ClaimedBuffer>(
                buffer,
                [weak_data, i]
                {
                    if (auto const locked = weak_data.lock())
                    {
                        std::lock_guard l{locked->mutex};
                        locked->buffers[i].first = false;
                    }
                });
        }
    }

    // All slots in use — grow the pool.
    data->buffers.emplace_back(true, data->builder());
    size_t const i = data->buffers.size() - 1;
    std::weak_ptr weak_data = data;
    return std::make_shared<ClaimedBuffer>(
        data->buffers[i].second,
        [weak_data, i]
        {
            if (auto const locked = weak_data.lock())
            {
                std::lock_guard l{locked->mutex};
                locked->buffers[i].first = false;
            }
        });
}

void miral::SoftwareBufferPool::set_builder(BufferBuilder&& builder)
{
    std::lock_guard lock{data->mutex};
    data->builder = std::move(builder);
    for (auto& b : data->buffers)
        b = {b.first, data->builder()};
}

std::list<ms::StreamInfo> miral::create_stream_info()
{
    std::list<ms::StreamInfo> result;

    /// A mc::Stream that always reports having a submitted buffer so that
    /// BasicSurface::generate_renderables() includes the surface in every frame.
    class AlwaysHasSubmittedBufferStream : public mir::compositor::Stream
    {
    public:
        bool has_submitted_buffer() const override { return true; }
    };

    result.push_back({std::make_shared<AlwaysHasSubmittedBufferStream>(), geom::Displacement{}});
    return result;
}
