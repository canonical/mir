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

#include "mir/graphics/egl_wayland_allocator.h"

#include <boost/throw_exception.hpp>
#include <mutex>

#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/texture.h"
#include "mir/renderer/gl/context.h"
#include "mir/executor.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/graphics/egl_context_executor.h"

#include <GLES2/gl2.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
GLuint get_tex_id()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return tex;
}

geom::Size get_wl_buffer_size(wl_resource* buffer, mg::EGLExtensions const& ext)
{
    EGLint width, height;

    auto dpy = eglGetCurrentDisplay();
    if (ext.wayland(dpy).eglQueryWaylandBufferWL(dpy, buffer, EGL_WIDTH, &width) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query WaylandAllocator buffer width"));
    }
    if (ext.wayland(dpy).eglQueryWaylandBufferWL(dpy, buffer, EGL_HEIGHT, &height) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query WaylandAllocator buffer height"));
    }

    return geom::Size{width, height};
}

mg::gl::Texture::Layout get_texture_layout(
    wl_resource* resource,
    mg::EGLExtensions const& ext)
{
    EGLint inverted;
    auto dpy = eglGetCurrentDisplay();

    if (ext.wayland(dpy).eglQueryWaylandBufferWL(dpy, resource, EGL_WAYLAND_Y_INVERTED_WL, &inverted) == EGL_FALSE)
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

EGLint get_wl_egl_format(wl_resource* resource, mg::EGLExtensions const& ext)
{
    EGLint format;
    auto dpy = eglGetCurrentDisplay();

    if (ext.wayland(dpy).eglQueryWaylandBufferWL(dpy, resource, EGL_TEXTURE_FORMAT, &format) == EGL_FALSE)
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
        wl_resource* buffer,
        mg::EGLExtensions const& extensions,
        std::shared_ptr<mir::graphics::common::EGLContextExecutor> egl_delegate,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release)
        : tex{get_tex_id()},
          on_consumed{std::move(on_consumed)},
          on_release{std::move(on_release)},
          size_{get_wl_buffer_size(buffer, extensions)},
          layout_{get_texture_layout(buffer, extensions)},
          egl_format{get_wl_egl_format(buffer, extensions)},
          egl_delegate{std::move(egl_delegate)}
    {
        if (egl_format != EGL_TEXTURE_RGB && egl_format != EGL_TEXTURE_RGBA)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"YUV textures unimplemented"}));
        }
        eglBindAPI(EGL_OPENGL_ES_API);

        const EGLint image_attrs[] =
            {
                EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                EGL_WAYLAND_PLANE_WL, 0,
                EGL_NONE
            };

        auto const dpy = eglGetCurrentDisplay();

        auto egl_image = extensions.base(dpy).eglCreateImageKHR(
            eglGetCurrentDisplay(),
            EGL_NO_CONTEXT,
            EGL_WAYLAND_BUFFER_WL,
            buffer,
            image_attrs);

        if (egl_image == EGL_NO_IMAGE_KHR)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLImage"));

        glBindTexture(GL_TEXTURE_2D, tex);
        extensions.base(dpy).glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // tex is now an EGLImage sibling, so we can free the EGLImage without
        // freeing the backing data.
        extensions.base(dpy).eglDestroyImageKHR(eglGetCurrentDisplay(), egl_image);
    }

    ~WaylandTexBuffer()
    {
        egl_delegate->spawn(
            [tex = tex]()
            {
              glDeleteTextures(1, &tex);
            });

        on_release();
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
        return layout_;
    }

    void bind() override
    {
        glBindTexture(GL_TEXTURE_2D, tex);

        std::lock_guard lock(consumed_mutex);
        on_consumed();
        on_consumed = [](){};
    }

    auto tex_id() const -> GLuint override
    {
        return tex;
    }

    void add_syncpoint() override
    {
    }
private:
    GLuint const tex;

    std::mutex consumed_mutex;
    std::function<void()> on_consumed;
    std::function<void()> const on_release;

    geom::Size const size_;
    Layout const layout_;
    EGLint const egl_format;

    std::shared_ptr<mir::graphics::common::EGLContextExecutor> const egl_delegate;
};
}



void mg::wayland::bind_display(EGLDisplay egl_dpy, wl_display* wayland_dpy, EGLExtensions const& extensions)
{
    if (extensions.wayland(egl_dpy).eglBindWaylandDisplayWL(egl_dpy, wayland_dpy) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to bind Wayland EGL display"));
    }
}

void mg::wayland::unbind_display(EGLDisplay egl_dpy, wl_display* wayland_dpy, EGLExtensions const& extensions)
{
    if (extensions.wayland(egl_dpy).eglUnbindWaylandDisplayWL(egl_dpy, wayland_dpy) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to unbind Wayland EGL display"));
    }
}

auto mg::wayland::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release,
    mg::EGLExtensions const& extensions,
    std::shared_ptr<mir::graphics::common::EGLContextExecutor> egl_delegate) -> std::unique_ptr<mg::Buffer>
{
    return std::make_unique<WaylandTexBuffer>(
        buffer,
        extensions,
        std::move(egl_delegate),
        std::move(on_consumed),
        std::move(on_release));
}
