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

#include "mir/graphics/gl_format.h"
#include "mir/renderer/sw/pixel_source.h"
#include "shm_buffer.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/egl_context_executor.h"

#define MIR_LOG_COMPONENT "gfx-common"
#include "mir/log.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <boost/throw_exception.hpp>

#include <string.h>
#include <endian.h>

namespace mg=mir::graphics;
namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

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

bool mgc::ShmBuffer::supports(MirPixelFormat mir_format)
{
    GLenum gl_format, gl_type;
    return mg::get_gl_pixel_format(mir_format, gl_format, gl_type);
}

namespace
{
auto get_tex_id_on_context(mgc::EGLContextExecutor& egl_executor) -> std::shared_future<GLuint>
{
    auto tex_promise = std::make_shared<std::promise<GLuint>>();
    /* We don't guarantee that the ShmBuffer constructor will be called on a thread with
     * an EGL context current, but allocating a GL texture ID and tex parameter setup
     * requires a current context.
     *
     * We do have a *way* of running code with an EGL context current: the EGLContextExecutor.
     * Delegate this setup there.
     */
    egl_executor.spawn(
        [tex_promise]()
        {
            GLuint tex;
            glGenTextures(1, &tex);

            // Paranoia: Save the current value for the GL state that we're modifying...
            GLint previous_texture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

            glBindTexture(GL_TEXTURE_2D, tex);
            // The ShmBuffer *should* be immutable, so we can just set up the properties once
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // ...and then restore the previous GL state.
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture));

            /* We need these commands to be in the driver command stream before
             * anything which uses them can be executed (which will likely be
             * on another thread, in another EGLContext).
             *
             * glFlush() before the promise synchronisation point will ensure
             * those commands are visible to the driver before we try and use
             * their results.
             */
            glFlush();
            tex_promise->set_value(tex);
        });
    return tex_promise->get_future();
}
}

mgc::ShmBuffer::ShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& format,
    std::shared_ptr<EGLContextExecutor> egl_delegate)
    : size_{size},
      pixel_format_{format},
      egl_delegate{std::move(egl_delegate)},
      tex{get_tex_id_on_context(*this->egl_delegate)}
{
}

mgc::MemoryBackedShmBuffer::MemoryBackedShmBuffer(
    geom::Size const& size,
    MirPixelFormat const& pixel_format,
    std::shared_ptr<EGLContextExecutor> egl_delegate)
    : ShmBuffer(size, pixel_format, std::move(egl_delegate)),
      stride_{MIR_BYTES_PER_PIXEL(pixel_format) * size.width.as_uint32_t()},
      pixels{new unsigned char[stride_.as_int() * size.height.as_int()]}
{
}

mgc::ShmBuffer::~ShmBuffer() noexcept
{
    if (auto id = tex_id())
    {
        egl_delegate->spawn(
            [id]()
            {
                glDeleteTextures(1, &id);
            });
    }
}

geom::Size mgc::ShmBuffer::size() const
{
    return size_;
}

MirPixelFormat mgc::ShmBuffer::pixel_format() const
{
    return pixel_format_;
}

void mgc::ShmBuffer::upload_to_texture(void const* pixels, geom::Stride const& stride)
{
    GLenum format, type;

    if (mg::get_gl_pixel_format(pixel_format_, format, type))
    {
        auto const stride_in_px =
            stride.as_int() / MIR_BYTES_PER_PIXEL(pixel_format());
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
            size().width.as_int(), size().height.as_int(),
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
            id().as_value(),
            pixel_format());
    }
}

mg::NativeBufferBase* mgc::ShmBuffer::native_buffer_base()
{
    return this;
}

void mgc::ShmBuffer::bind()
{
    glBindTexture(GL_TEXTURE_2D, tex_id());
}

auto mgc::ShmBuffer::tex_id() const -> GLuint
{
    return tex.get();
}

void mgc::MemoryBackedShmBuffer::bind()
{
    mgc::ShmBuffer::bind();
    std::lock_guard lock{uploaded_mutex};
    if (!uploaded)
    {
        upload_to_texture(pixels.get(), stride_);
        uploaded = true;
    }
}

void mgc::MemoryBackedShmBuffer::mark_dirty()
{
    std::lock_guard lock{uploaded_mutex};
    uploaded = false;
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

    auto data() -> T* override
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

auto mgc::MemoryBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return std::make_unique<Mapping<unsigned char>>(this);
}

auto mgc::MemoryBackedShmBuffer::map_readable() -> std::unique_ptr<mrs::Mapping<unsigned char const>>
{
    return std::make_unique<Mapping<unsigned char const>>(this);
}

auto mgc::MemoryBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return std::make_unique<Mapping<unsigned char>>(this);
}

mg::gl::Program const& mgc::ShmBuffer::shader(mg::gl::ProgramFactory& cache) const
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

auto mgc::ShmBuffer::layout() const -> Layout
{
    return Layout::GL;
}

void mgc::ShmBuffer::add_syncpoint()
{
}

mgc::MappableBackedShmBuffer::MappableBackedShmBuffer(
    std::shared_ptr<mrs::RWMappableBuffer> data,
    std::shared_ptr<EGLContextExecutor> egl_delegate)
    : ShmBuffer(data->size(), data->format(), std::move(egl_delegate)),
      data{std::move(data)}
{
}

auto mgc::MappableBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return data->map_writeable();
}

auto mgc::MappableBackedShmBuffer::map_readable() -> std::unique_ptr<mrs::Mapping<unsigned char const>>
{
    return data->map_readable();
}

auto mgc::MappableBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    return data->map_rw();
}

void mgc::MappableBackedShmBuffer::bind()
{
    mgc::ShmBuffer::bind();
    std::lock_guard lock{uploaded_mutex};
    if (!uploaded)
    {
        auto mapping = data->map_readable();
        upload_to_texture(mapping->data(), mapping->stride());
        uploaded = true;
    }
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
    std::shared_ptr<mrs::RWMappableBuffer> data,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
    :  MappableBackedShmBuffer(std::move(data), std::move(egl_delegate)),
       on_consumed{std::move(on_consumed)},
       on_release{std::move(on_release)}
{
}

mgc::NotifyingMappableBackedShmBuffer::~NotifyingMappableBackedShmBuffer()
{
    on_release();
}

void mgc::NotifyingMappableBackedShmBuffer::notify_consumed()
{
    std::lock_guard lock{consumed_mutex};
    on_consumed();
    on_consumed = [](){};
}

void mgc::NotifyingMappableBackedShmBuffer::bind()
{
    MappableBackedShmBuffer::bind();
    notify_consumed();
}

auto mgc::NotifyingMappableBackedShmBuffer::map_readable() -> std::unique_ptr<mrs::Mapping<unsigned char const>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_readable();
}

auto mgc::NotifyingMappableBackedShmBuffer::map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_writeable();
}

auto mgc::NotifyingMappableBackedShmBuffer::map_rw() -> std::unique_ptr<mrs::Mapping<unsigned char>>
{
    notify_consumed();
    return MappableBackedShmBuffer::map_rw();
}
