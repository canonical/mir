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

#include "mir/graphics/buffer_id.h"
#include "mir/graphics/gl_format.h"
#include "mir/graphics/ptr_backed_mapping.h"
#include "mir/graphics/texture.h"
#include "mir/renderer/sw/pixel_source.h"
#include "shm_buffer.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir_toolkit/common.h"

#define MIR_LOG_COMPONENT "gfx-common"
#include "mir/log.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <boost/throw_exception.hpp>

namespace mg=mir::graphics;
namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

namespace
{
GLuint new_texture()
{
    GLuint tex;
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    // The ShmBuffer *should* be immutable, so we can just set up the properties once
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}
}

bool mg::get_gl_pixel_format(MirPixelFormat mir_format,
                         GLenum& gl_format, GLenum& gl_type)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    GLenum const argb = GL_BGRA_EXT;
    GLenum const abgr = GL_RGBA;
#elif __BYTE_ORDER == __BIG_ENDIAN
    // TODO: Big endian support
    GLenum const argb = GL_INVALID_ENUM;
    GLenum const abgr = GL_INVALID_ENUM;
    //GLenum const rgba = GL_RGBA;
    //GLenum const bgra = GL_BGRA_EXT;
#endif

    static const struct
    {
        MirPixelFormat mir_format;
        GLenum gl_format, gl_type;
    } mapping[mir_pixel_formats] =
    {
        {mir_pixel_format_invalid,   GL_INVALID_ENUM, GL_INVALID_ENUM},
        {mir_pixel_format_abgr_8888, abgr,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_xbgr_8888, abgr,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_argb_8888, argb,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_xrgb_8888, argb,            GL_UNSIGNED_BYTE},
        {mir_pixel_format_bgr_888,   GL_INVALID_ENUM, GL_INVALID_ENUM},
        {mir_pixel_format_rgb_888,   GL_RGB,          GL_UNSIGNED_BYTE},
        {mir_pixel_format_rgb_565,   GL_RGB,          GL_UNSIGNED_SHORT_5_6_5},
        {mir_pixel_format_rgba_5551, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1},
        {mir_pixel_format_rgba_4444, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4},
    };

    if (mir_format > mir_pixel_format_invalid &&
        mir_format < mir_pixel_formats &&
        mapping[mir_format].mir_format == mir_format) // just a sanity check
    {
        gl_format = mapping[mir_format].gl_format;
        gl_type = mapping[mir_format].gl_type;
    }
    else
    {
        gl_format = GL_INVALID_ENUM;
        gl_type = GL_INVALID_ENUM;
    }

    return gl_format != GL_INVALID_ENUM && gl_type != GL_INVALID_ENUM;
}

class mgc::ShmBuffer::ShmBufferTexture : public gl::Texture
{
public:
    explicit ShmBufferTexture(std::shared_ptr<EGLContextExecutor> const& egl_delegate)
        : egl_delegate(egl_delegate),
          tex_id_(new_texture())
    {
    }

    ~ShmBufferTexture() override
    {
        egl_delegate->spawn(
            [id=tex_id()]
            {
                glDeleteTextures(1, &id);
            });
    }

    void bind() override
    {
        glBindTexture(GL_TEXTURE_2D, tex_id());
    }

    auto tex_id() const -> GLuint override
    {
        return tex_id_;
    }

    gl::Program const& shader(mg::gl::ProgramFactory& cache) const override
    {
        static int argb_shader{0};
        return cache.compile_fragment_shader(
            &argb_shader,
            "",
            "uniform sampler2D tex;\n"
            "vec4 sample_to_rgba(in vec2 texcoord)\n"
            "{\n"
            "    return texture2D(tex, texcoord);\n"
            "}\n");
    }

    Layout layout() const override
    {
        return Layout::GL;
    }

    void add_syncpoint() override
    {
    }

    void try_upload_to_texture(
        BufferID id, void const* pixels, geometry::Size const& size,
        geom::Stride const& stride, MirPixelFormat pixel_format)
    {
        std::lock_guard lock{uploaded_mutex};
        if (uploaded)
            return;

        bind();
        GLenum format, type;

        if (mg::get_gl_pixel_format(pixel_format, format, type))
        {
            auto const stride_in_px =
                stride.as_int() / MIR_BYTES_PER_PIXEL(pixel_format);
            /*
             * We assume (as does Weston, AFAICT) that stride is
             * a multiple of whole pixels, but it need not be.
             *
             * TODO: Handle non-pixel-multiple strides.
             * This should be possible by calculating GL_UNPACK_ALIGNMENT
             * to match the size of the partial-pixel-stride().
             */

            glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride_in_px);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                format,
                size.width.as_int(), size.height.as_int(),
                0,
                format,
                type,
                pixels);

            // Be nice to other users of the GL context by reverting our changes to shared state
            glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);     // 0 is default, meaning “use width”
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);          // 4 is default; word alignment.
            glFinish();
        }
        else
        {
            mir::log_error(
                "Buffer %i has non-GL-compatible pixel format %i; rendering will be incomplete",
                id.as_value(),
                pixel_format);
        }

        uploaded = true;
    }

    void mark_dirty()
    {
        std::lock_guard lock{uploaded_mutex};
        uploaded = false;
    }

private:
    std::shared_ptr<EGLContextExecutor> egl_delegate;
    GLuint tex_id_;
    std::mutex uploaded_mutex;
    bool uploaded = false;
};

auto mgc::ShmBuffer::texture_from_mapping(
    std::shared_ptr<mgc::EGLContextExecutor> executor,
    std::shared_ptr<mrs::Mapping<const std::byte>> mapping)
    -> std::shared_ptr<gl::Texture>
{
    auto tex = std::shared_ptr<ShmBufferTexture>{
        new ShmBufferTexture(std::move(executor)),
        [mapping](auto to_delete) { delete to_delete; }};
    tex->try_upload_to_texture(mg::BufferID{9}, mapping->data(), mapping->size(), mapping->stride(), mapping->format());
    return tex;
}


bool mgc::ShmBuffer::supports(MirPixelFormat mir_format)
{
    GLenum gl_format, gl_type;
    return mg::get_gl_pixel_format(mir_format, gl_format, gl_type);
}

mgc::ShmBuffer::ShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& format)
    : size_{size},
      pixel_format_{format}
{
}

mgc::ShmBuffer::~ShmBuffer() noexcept
{
}

geom::Size mgc::ShmBuffer::size() const
{
    return size_;
}

MirPixelFormat mgc::ShmBuffer::pixel_format() const
{
    return pixel_format_;
}

mg::NativeBufferBase* mgc::ShmBuffer::native_buffer_base()
{
    return this;
}

auto mgc::ShmBuffer::texture_for_provider(
    std::shared_ptr<EGLContextExecutor> const& egl_delegate,
    RenderingProvider* provider) -> std::shared_ptr<gl::Texture>
{
    auto const locked_provider_to_texture_map = provider_to_texture_map.lock();
    // This method is called from the renderer where the egl context is current.
    // Hence, we do not need to spawn texture creation the egl_delegate.
    if (!locked_provider_to_texture_map->contains(provider))
        locked_provider_to_texture_map->emplace(provider, std::make_shared<ShmBufferTexture>(egl_delegate));

    auto texture = locked_provider_to_texture_map->at(provider);
    on_texture_accessed(texture);
    return texture;
}

void mgc::ShmBuffer::on_texture_accessed(std::shared_ptr<ShmBufferTexture> const&)
{
}

mgc::MemoryBackedShmBuffer::MemoryBackedShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& pixel_format)
    : ShmBuffer(size, pixel_format),
      stride_{MIR_BYTES_PER_PIXEL(pixel_format) * size.width.as_uint32_t()},
      pixels{std::make_unique<std::byte[]>(stride_.as_int() * size.height.as_int())}
{
}

auto mgc::MemoryBackedShmBuffer::map_readable() const -> std::unique_ptr<mrs::Mapping<std::byte const>>
{
    return std::make_unique<PtrBackedMapping<std::byte const>>(
        pixels.get(),
        format(),
        size(),
        stride());
}

void mgc::MemoryBackedShmBuffer::on_texture_accessed(std::shared_ptr<ShmBufferTexture> const& texture)
{
    texture->try_upload_to_texture(
        id(),
        pixels.get(),
        size(),
        stride_,
        pixel_format());
}

void mgc::MemoryBackedShmBuffer::mark_dirty()
{
    auto const locked_provider_to_texture_map = provider_to_texture_map.lock();
    for (auto const& [provider, texture] : *locked_provider_to_texture_map)
        texture->mark_dirty();
}

template<typename T>
class mgc::MemoryBackedShmBuffer::Mapping : public mir::renderer::software::Mapping<T>
{
public:
    Mapping(std::conditional_t<std::is_const_v<T>, MemoryBackedShmBuffer const*, MemoryBackedShmBuffer*> buffer)
        : buffer{buffer}
    {
    }

    ~Mapping() override
    {
        if constexpr (!std::is_const_v<T>)
        {
            buffer->mark_dirty();
        }
    }

    auto format() const -> MirPixelFormat override
    {
        return buffer->pixel_format();
    }

    auto stride() const -> geom::Stride override
    {
        return buffer->stride_;
    }

    auto size() const -> geom::Size override
    {
        return buffer->size();
    }

    auto data() const -> T* override
    {
        return buffer->pixels.get();
    }

    auto len() const -> size_t override
    {
        return stride().as_uint32_t() * size().height.as_uint32_t();
    }

private:
    std::conditional_t<std::is_const_v<T>, MemoryBackedShmBuffer const*, MemoryBackedShmBuffer*> buffer;
};

auto mgc::MemoryBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return std::make_unique<Mapping<std::byte>>(this);
}

auto mgc::MemoryBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return std::make_unique<Mapping<std::byte>>(this);
}

mgc::MappableBackedShmBuffer::MappableBackedShmBuffer(
    std::shared_ptr<mrs::RWMappable> data)
    : ShmBuffer(data->size(), data->format()),
      data{std::move(data)}
{
}

auto mgc::MappableBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return data->map_writeable();
}

auto mgc::MappableBackedShmBuffer::map_readable() const -> std::unique_ptr<mrs::Mapping<std::byte const>>
{
    class ReadOnlyWrapper : public mrs::Mapping<std::byte const>
    {
    public:
        explicit ReadOnlyWrapper(std::unique_ptr<mrs::Mapping<std::byte>> mapping)
            : mapping{std::move(mapping)}
        {
        }

        auto data() const -> std::byte const* override
        {
            return mapping->data();
        }

        auto len() const -> size_t override
        {
            return mapping->len();
        }

        auto format() const -> MirPixelFormat override
        {
            return mapping->format();
        }

        auto stride() const -> geom::Stride override
        {
            return mapping->stride();
        }

        auto size() const -> geom::Size override
        {
            return mapping->size();
        }
    private:
        std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
    };
    return std::make_unique<ReadOnlyWrapper>(data->map_rw());
}

auto mgc::MappableBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    return data->map_rw();
}

void mgc::MappableBackedShmBuffer::on_texture_accessed(std::shared_ptr<ShmBufferTexture> const& texture)
{
    auto mapping = map_readable();
    texture->try_upload_to_texture(
        id(),
        mapping->data(),
        size(),
        stride(),
        pixel_format());
}

auto mgc::MappableBackedShmBuffer::format() const -> MirPixelFormat
{
    return data->format();
}

auto mgc::MappableBackedShmBuffer::stride() const -> geometry::Stride
{
    return data->stride();
}

auto mgc::MappableBackedShmBuffer::size() const -> geometry::Size
{
    return data->size();
}

mgc::NotifyingMappableBackedShmBuffer::NotifyingMappableBackedShmBuffer(
    std::shared_ptr<mrs::RWMappable> data,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
    :  MappableBackedShmBuffer(std::move(data)),
       on_consumed{std::move(on_consumed)},
       on_release{std::move(on_release)}
{
}

mgc::NotifyingMappableBackedShmBuffer::~NotifyingMappableBackedShmBuffer()
{
    on_release();
}

void mgc::NotifyingMappableBackedShmBuffer::notify_consumed() const
{
    auto consumed = on_consumed.lock_mut();
    (*consumed)();
    *consumed = [](){};
}

auto mgc::NotifyingMappableBackedShmBuffer::map_readable() const -> std::unique_ptr<mrs::Mapping<std::byte const>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_readable();
}

auto mgc::NotifyingMappableBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_writeable();
}

auto mgc::NotifyingMappableBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<std::byte>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_rw();
}

void mgc::NotifyingMappableBackedShmBuffer::on_texture_accessed(std::shared_ptr<ShmBufferTexture> const& texture)
{
    MappableBackedShmBuffer::on_texture_accessed(texture);
    notify_consumed();
}
