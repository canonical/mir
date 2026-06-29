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

#include "software_buffer_stream.h"

#include <mir/geometry/displacement.h>

namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;
namespace ms = mir::scene;

miral::ClaimedBuffer::ClaimedBuffer(
    std::shared_ptr<mg::Buffer> const& buffer,
    std::function<void()>&& release_callback) :
    buffer(buffer),
    release_callback(std::move(release_callback))
{}

miral::ClaimedBuffer::~ClaimedBuffer() { release_callback(); }

auto miral::ClaimedBuffer::map_readable() const -> std::unique_ptr<mrs::Mapping<std::byte const>>
{ return buffer->map_readable(); }

mg::BufferID miral::ClaimedBuffer::id() const { return buffer->id(); }
geom::Size miral::ClaimedBuffer::size() const { return buffer->size(); }
MirPixelFormat miral::ClaimedBuffer::pixel_format() const { return buffer->pixel_format(); }
mg::NativeBufferBase* miral::ClaimedBuffer::native_buffer_base() { return buffer->native_buffer_base(); }

miral::BufferPool::Self::Self(BufferBuilder&& builder_func, size_t initial_size) : builder(std::move(builder_func))
{
    for (size_t i = 0; i < initial_size; ++i)
        buffers.emplace_back(false, builder());
}

miral::BufferPool::BufferPool(BufferBuilder&& builder_func, size_t initial_size) :
    data(std::make_shared<Self>(std::move(builder_func), initial_size))
{}

std::shared_ptr<mg::Buffer> miral::BufferPool::claim()
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

void miral::BufferPool::set_builder(BufferBuilder&& builder)
{
    std::lock_guard lock{data->mutex};
    data->builder = std::move(builder);
    for (auto& b : data->buffers)
        b = {b.first, data->builder()};
}

std::list<ms::StreamInfo> miral::create_stream_info()
{
    std::list<ms::StreamInfo> result;
    result.push_back({std::make_shared<AlwaysHasSubmittedBufferStream>(), geom::Displacement{}});
    return result;
}
