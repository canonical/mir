/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include "mir/raii.h"
#include "mir/anonymous_shm_file.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/executor.h"
#include "shm_buffer.h"
#include "buffer_allocator.h"

#define MIR_LOG_COMPONENT "rpi-dispmanx"
#include <mir/log.h>


namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{
std::unique_ptr<mir::renderer::gl::Context> context_for_output(mg::Display const& output)
{
    try
    {
        auto& context_source = dynamic_cast<mir::renderer::gl::ContextSource const&>(output);

        /*
         * We care about no part of this context's config; we will do no rendering with it.
         * All we care is that we can allocate texture IDs and bind a texture, which is
         * config independent.
         *
         * That's not *entirely* true; we also need it to be on the same device as we want
         * to do the rendering on, and that GL must support all the extensions we care about,
         * but since we don't yet support heterogeneous hybrid and implementing that will require
         * broader interface changes it's a safe enough requirement for now.
         */
        return context_source.create_gl_context();
    }
    catch (std::bad_cast const& err)
    {
        std::throw_with_nested(
            boost::enable_error_info(
                std::runtime_error{"Output platform cannot provide a GL context"})
                << boost::throw_function(__PRETTY_FUNCTION__)
                << boost::throw_line(__LINE__)
                << boost::throw_file(__FILE__));
    }
}

}

mg::rpi::BufferAllocator::BufferAllocator(mir::graphics::Display const& output)
    : egl_extensions{std::make_shared<mg::EGLExtensions>()},
      ctx{context_for_output(output)}
{
}

auto mg::rpi::BufferAllocator::alloc_buffer(mg::BufferProperties const&)
    -> std::shared_ptr<Buffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"Platform does not support deprecated alloc_buffer()"}));
}
auto mg::rpi::BufferAllocator::supported_pixel_formats()
    -> std::vector<MirPixelFormat>
{
    return {mir_pixel_format_argb_8888};
}
auto mg::rpi::BufferAllocator::alloc_buffer(
    mir::geometry::Size, uint32_t, uint32_t)
    -> std::shared_ptr<Buffer>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"Platform does not support mirclient"}));
}
auto mg::rpi::BufferAllocator::alloc_software_buffer(
    mir::geometry::Size size, MirPixelFormat format)
    -> std::shared_ptr<Buffer>
{
    if (!common::ShmBuffer::supports(format))
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{"Trying to create SHM buffer with unsupported pixel format"}));
    }

    class SimpleShmBuffer : public common::ShmBuffer
    {
    public:
        SimpleShmBuffer(
            geom::Size size,
            MirPixelFormat const& pixelFormat) :
            ShmBuffer(
                std::make_unique<mir::AnonymousShmFile>(size_in_bytes(size, pixelFormat)),
                size,
                pixelFormat)
        {
        }

        std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"rpi-dispmanx platform does not support mirclient"}));
        }

    private:
        static auto size_in_bytes(geom::Size size, MirPixelFormat format) -> size_t
        {
            auto const stride = geom::Stride{ MIR_BYTES_PER_PIXEL(format) * size.width.as_uint32_t() };
            return stride.as_int() * size.height.as_int();
        }
    };

    return std::make_shared<SimpleShmBuffer>(size, format);
}

namespace
{
GLuint get_tex_id()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return tex;
}

mir::geometry::Size get_wl_buffer_size(wl_resource* buffer, mg::EGLExtensions::WaylandExtensions const& ext)
{
    EGLint width, height;

    auto dpy = eglGetCurrentDisplay();
    if (ext.eglQueryWaylandBufferWL(dpy, buffer, EGL_WIDTH, &width) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query WaylandAllocator buffer width"));
    }
    if (ext.eglQueryWaylandBufferWL(dpy, buffer, EGL_HEIGHT, &height) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query WaylandAllocator buffer height"));
    }

    return geom::Size{width, height};
}

mg::gl::Texture::Layout get_texture_layout(
    wl_resource* resource,
    mg::EGLExtensions::WaylandExtensions const& ext)
{
    EGLint inverted;
    auto dpy = eglGetCurrentDisplay();

    if (ext.eglQueryWaylandBufferWL(dpy, resource, EGL_WAYLAND_Y_INVERTED_WL, &inverted) == EGL_FALSE)
    {
        // EGL_WAYLAND_Y_INVERTED_WL is unsupported; the default is that the texture is in standard
        // GL texture layout
        return mg::gl::Texture::Layout::GL;
    }
    if (inverted)
    {
        // It has the standard y-decreases-with-row layout of GL textures
        return mg::gl::Texture::Layout::GL;
    }
    else
    {
        // It has y-increases-with-row layout.
        return mg::gl::Texture::Layout::TopRowFirst;
    }
}

EGLint get_wl_egl_format(wl_resource* resource, mg::EGLExtensions::WaylandExtensions const& ext)
{
    EGLint format;
    auto dpy = eglGetCurrentDisplay();

    if (ext.eglQueryWaylandBufferWL(dpy, resource, EGL_TEXTURE_FORMAT, &format) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query Wayland buffer format"));
    }
    return format;
}

class WaylandTexBuffer :
    public mg::BufferBasic,
    public mg::NativeBufferBase,
    public mg::gl::Texture
{
public:
    // Note: Must be called with a current EGL context
    WaylandTexBuffer(
        std::shared_ptr<mir::renderer::gl::Context> ctx,
        wl_resource* buffer,
        mg::EGLExtensions const& extensions,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release,
        std::shared_ptr<mir::Executor> wayland_executor)
        : ctx{std::move(ctx)},
          tex{get_tex_id()},
          on_consumed{std::move(on_consumed)},
          on_release{std::move(on_release)},
          size_{get_wl_buffer_size(buffer, *extensions.wayland)},
          layout_{get_texture_layout(buffer, *extensions.wayland)},
          egl_format{get_wl_egl_format(buffer, *extensions.wayland)},
          wayland_executor{std::move(wayland_executor)}
    {
        if (egl_format != EGL_TEXTURE_RGB && egl_format != EGL_TEXTURE_RGBA)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"YUV textures unimplemented"}));
        }
        eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

        const EGLint image_attrs[] =
            {
                EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                EGL_WAYLAND_PLANE_WL, 0,
                EGL_NONE
            };

        auto egl_image = extensions.eglCreateImageKHR(
            eglGetCurrentDisplay(),
            EGL_NO_CONTEXT,
            EGL_WAYLAND_BUFFER_WL,
            buffer,
            image_attrs);

        if (egl_image == EGL_NO_IMAGE_KHR)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLImage"));

        glBindTexture(GL_TEXTURE_2D, tex);
        extensions.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // tex is now an EGLImage sibling, so we can free the EGLImage without
        // freeing the backing data.
        extensions.eglDestroyImageKHR(eglGetCurrentDisplay(), egl_image);
    }

    ~WaylandTexBuffer()
    {
        wayland_executor->spawn(
            [context = ctx, tex = tex]()
            {
                context->make_current();

                glDeleteTextures(1, &tex);

                context->release_current();
            });

        on_release();
    }

    std::shared_ptr<mir::graphics::NativeBuffer> native_buffer_handle() const override
    {
        return {nullptr};
    }

    mir::geometry::Size size() const override
    {
        return size_;
    }

    MirPixelFormat pixel_format() const override
    {
        /* TODO: These are lies, but the only piece of information external code uses
         * out of the MirPixelFormat is whether or not the buffer has an alpha channel.
         */
        switch(egl_format)
        {
            case EGL_TEXTURE_RGB:
                return mir_pixel_format_xrgb_8888;
            case EGL_TEXTURE_RGBA:
                return mir_pixel_format_argb_8888;
            case EGL_TEXTURE_EXTERNAL_WL:
                // Unspecified whether it has an alpha channel; say it does.
                return mir_pixel_format_argb_8888;
            case EGL_TEXTURE_Y_U_V_WL:
            case EGL_TEXTURE_Y_UV_WL:
                // These are just absolutely not RGB at all!
                // But they're defined to not have an alpha channel, so xrgb it is!
                return mir_pixel_format_xrgb_8888;
            case EGL_TEXTURE_Y_XUXV_WL:
                // This is a planar format, but *does* have alpha.
                return mir_pixel_format_argb_8888;
            default:
                // We've covered all possibilities above
                BOOST_THROW_EXCEPTION((std::logic_error{"Unexpected texture format!"}));
        }
    }

    NativeBufferBase* native_buffer_base() override
    {
        return this;
    }

    mir::graphics::gl::Program const& shader(mir::graphics::gl::ProgramFactory& cache) const override
    {
        static int rgba_shader_tag{0};
        return cache.compile_fragment_shader(
            &rgba_shader_tag,
            "",
            "uniform sampler2D tex;\n"
            "vec4 sample_to_rgba(in vec2 texcoord)\n"
            "{\n"
            "    return texture2D(tex, texcoord);\n"
            "}\n");
    }

    Layout layout() const override
    {
        return layout_;
    }

    void bind() override
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        on_consumed();
        on_consumed = [](){};
    }

    void add_syncpoint() override
    {
    }
private:
    std::shared_ptr<mir::renderer::gl::Context> const ctx;
    GLuint const tex;

    std::function<void()> on_consumed;
    std::function<void()> const on_release;

    geom::Size const size_;
    Layout const layout_;
    EGLint const egl_format;

    std::shared_ptr<mir::Executor> const wayland_executor;
};
}

void mg::rpi::BufferAllocator::bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });
    auto dpy = eglGetCurrentDisplay();

    if (!egl_extensions->wayland)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"No EGL_WL_bind_wayland_display support"}));
    }

    if (egl_extensions->wayland->eglBindWaylandDisplayWL(dpy, display) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to bind Wayland EGL display"));
    }
    else
    {
        mir::log_info("Bound WaylandAllocator display");
    }
    this->wayland_executor = std::move(wayland_executor);
}

std::shared_ptr<mg::Buffer> mg::rpi::BufferAllocator::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release)
{
    auto context_guard = mir::raii::paired_calls(
        [this]() { ctx->make_current(); },
        [this]() { ctx->release_current(); });

    return std::make_shared<WaylandTexBuffer>(
        ctx,
        buffer,
        *egl_extensions,
        std::move(on_consumed),
        std::move(on_release),
        wayland_executor);
}

