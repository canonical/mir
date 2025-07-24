/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "mir/graphics/graphic_buffer_allocator.h"

namespace mir::graphics::common
{
class EGLContextExecutor;
}

namespace mir::graphics::gl
{
class Texture;
}

namespace mir::graphics
{
class RenderingProvider;
}

namespace mir::graphics::shm
{
class ShmBufferStorage : public BufferStorage
{
public:
    class Mapped : public GraphicBufferAllocator::MappedStorage
    {
    };
};


auto alloc_buffer_storage(BufferParams const& params) -> std::unique_ptr<ShmBufferStorage>;

auto map_rw(std::unique_ptr<ShmBufferStorage> storage)
  -> std::unique_ptr<GraphicBufferAllocator::MappedStorage>;
auto map_writeable(std::unique_ptr<ShmBufferStorage> storage)
  -> std::unique_ptr<GraphicBufferAllocator::MappedStorage>;

auto commit(std::unique_ptr<ShmBufferStorage::Mapped> mapping)
  -> std::unique_ptr<BufferStorage>;

auto into_buffer(
    std::unique_ptr<ShmBufferStorage> storage,
    std::function<void(std::unique_ptr<BufferStorage>)> on_return) -> std::shared_ptr<Buffer>;

auto as_texture(
    std::shared_ptr<Buffer> buffer,
    std::shared_ptr<common::EGLContextExecutor> const& egl_delegate,
    RenderingProvider* provider) -> std::shared_ptr<gl::Texture>;
}
