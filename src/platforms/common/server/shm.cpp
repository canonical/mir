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

#include "shm.h"
#include "mir_toolkit/common.h"
#include "shm_buffer.h"

#include "mir/renderer/sw/pixel_source.h"

#include <boost/throw_exception.hpp>
#include <drm_fourcc.h>

namespace mg = mir::graphics;
namespace mgs = mir::graphics::shm;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

namespace mir::graphics::common
{
class EGLContextExecutor;
}

namespace
{
class Storage : public mgs::ShmBufferStorage
{
public:
    Storage(geom::Size size, mg::DRMFormat format, geom::Stride stride)
      : buffer{size, format.as_mir_format().value_or(mir_pixel_format_invalid), stride}
    {
    }

    mg::common::MemoryBackedShmBuffer buffer;
};

class Mapping : public mgs::ShmBufferStorage::Mapped
{
public:
    Mapping(
        std::unique_ptr<mgs::ShmBufferStorage> storage,
        std::unique_ptr<mrs::Mapping<unsigned char>> mapping)
      : storage{std::move(storage)},
        underlying_mapping{{std::move(mapping)}}
    {
    }

    auto data() -> std::byte* override
    {
        return reinterpret_cast<std::byte*>(underlying_mapping->data());
    }
    auto len() const -> size_t override
    {
        return underlying_mapping->len();
    }

    auto format() const -> MirPixelFormat override
    {
        return underlying_mapping->format();
    }
    auto stride() const -> geom::Stride override
    {
        return underlying_mapping->stride();
    }
    auto size() const -> geom::Size override
    {
        return underlying_mapping->size();
    }

    std::unique_ptr<mgs::ShmBufferStorage> storage;
    std::unique_ptr<mrs::Mapping<unsigned char>> const underlying_mapping;
};

template<typename To, typename From>
auto unique_ptr_cast(std::unique_ptr<From> ptr) -> std::unique_ptr<To>
{
    From* unowned_src = ptr.release();
    if (auto to_src = dynamic_cast<To*>(unowned_src))
    {
        return std::unique_ptr<To>{to_src};
    }
    delete unowned_src;
    BOOST_THROW_EXCEPTION((
        std::bad_cast()));
}
}

auto mgs::alloc_buffer_storage(BufferParams const& params) -> std::unique_ptr<ShmBufferStorage>
{
    auto const size = params.size();
    auto const format =
        params.format().value_or(mg::DRMFormat{DRM_FORMAT_ARGB8888});

    auto const stride =
        params.stride().value_or(
            [&format, &size]()
            {
                if (!format.info() || !format.info()->bits_per_pixel())
                {
                    BOOST_THROW_EXCEPTION((std::invalid_argument{""}));
                }
                return geom::Stride{size.width.as_uint32_t() * (*(format.info()->bits_per_pixel()) / 8)};
            }());

    return std::make_unique<Storage>(size, format, stride);  
}

auto mgs::map_rw(std::unique_ptr<ShmBufferStorage> storage)
  -> std::unique_ptr<GraphicBufferAllocator::MappedStorage>
{
    auto cast_storage = unique_ptr_cast<Storage>(std::move(storage));
    auto mapping = cast_storage->buffer.map_rw();
    return std::make_unique<Mapping>(std::move(cast_storage), std::move(mapping));
}

auto mgs::map_writeable(std::unique_ptr<ShmBufferStorage> storage)
  -> std::unique_ptr<GraphicBufferAllocator::MappedStorage>
{
    auto cast_storage = unique_ptr_cast<Storage>(std::move(storage));
    auto mapping = cast_storage->buffer.map_writeable();
    return std::make_unique<Mapping>(std::move(cast_storage), std::move(mapping));
}

auto mgs::commit(std::unique_ptr<ShmBufferStorage::Mapped> mapping)
  -> std::unique_ptr<BufferStorage>
{
    auto cast_mapping = unique_ptr_cast<Mapping>(std::move(mapping));
    return std::move(cast_mapping->storage);
}

namespace
{
class StorageBackedBuffer : public mg::BufferBasic, public mg::NativeBufferBase
{
public:
    StorageBackedBuffer(
        std::unique_ptr<Storage> storage,
        std::function<void(std::unique_ptr<mg::BufferStorage>)> on_return)
        : storage{std::move(storage)},
          on_return{std::move(on_return)}
    {
    }

    ~StorageBackedBuffer()
    {
        on_return(std::move(storage));
    }
    
    auto size() const -> geom::Size override
    {
        return storage->buffer.size();
    }
    auto pixel_format() const -> MirPixelFormat override
    {
        return storage->buffer.pixel_format();
    }

    auto native_buffer_base() -> mg::NativeBufferBase* override
    {
        return this;
    }

    auto texture_for_provider(
        std::shared_ptr<mg::common::EGLContextExecutor> const& delegate,
        mg::RenderingProvider* provider) -> std::shared_ptr<mg::gl::Texture>
    {
        return storage->buffer.texture_for_provider(delegate, provider);
    }
private:
    std::unique_ptr<Storage> storage;
    std::function<void(std::unique_ptr<mg::BufferStorage>)> const on_return;
};  
}

auto mgs::into_buffer(
    std::unique_ptr<ShmBufferStorage> storage,
    std::function<void(std::unique_ptr<BufferStorage>)> on_return) -> std::shared_ptr<Buffer>
{ 
    auto cast_mapping = unique_ptr_cast<Storage>(std::move(storage));
    return std::make_shared<StorageBackedBuffer>(std::move(cast_mapping), std::move(on_return));
}

auto mgs::as_texture(
    std::shared_ptr<Buffer> buffer,
    std::shared_ptr<common::EGLContextExecutor> const& egl_delegate,
    RenderingProvider* provider) -> std::shared_ptr<gl::Texture>
{
    auto native_buffer = std::shared_ptr<NativeBufferBase>{buffer, buffer->native_buffer_base()};
    if (auto our_buf = std::dynamic_pointer_cast<StorageBackedBuffer>(native_buffer))
    {
        return our_buf->texture_for_provider(egl_delegate, provider);
    }
    return nullptr;
}
