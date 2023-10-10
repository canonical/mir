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
 *
 * Authored by: Christopher James Halse Rogers
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <drm_fourcc.h>

#include "mir/graphics/egl_error.h"
#include "mir/graphics/platform.h"
#include "mir/log.h"

#include "cpu_copy_output_surface.h"

namespace mg = mir::graphics;
namespace mgc = mg::common;
namespace geom = mir::geometry;

namespace
{
template<void (*allocator)(GLsizei, GLuint*), void (* deleter)(GLsizei, GLuint const*)>
class GLHandle
{
public:
    GLHandle()
    {
        (*allocator)(1, &id);
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using RenderbufferHandle = GLHandle<&glGenRenderbuffers, &glDeleteRenderbuffers>;
using FramebufferHandle = GLHandle<&glGenFramebuffers, &glDeleteFramebuffers>;

auto create_current_context(EGLDisplay dpy, EGLContext share_ctx)
    -> EGLContext
{
    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    auto egl_extensions = eglQueryString(dpy, EGL_EXTENSIONS);
    if (strstr(egl_extensions, "EGL_KHR_no_config_context") == nullptr)
    {
        // We do not *strictly* need this, but it means I don't need to thread a GLConfig all the way through to here.
        BOOST_THROW_EXCEPTION((std::runtime_error{"EGL implementation missing necessary EGL_KHR_no_config_context extension"}));
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    auto ctx = eglCreateContext(dpy, EGL_NO_CONFIG_KHR, share_ctx, context_attr);

    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
    }
    return ctx;
}

auto select_format_from(mg::CPUAddressableDisplayProvider const& provider) -> mg::DRMFormat
{
    std::optional<mg::DRMFormat> best_format;
    for (auto const format : provider.supported_formats())
    {
        switch(static_cast<uint32_t>(format))
        {
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_XRGB8888:
            // ?RGB8888 is the easiest for us
            return format;
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_RGBX8888:
            // RGB?8888 requires an EGL extension, but is OK
            best_format = format;
            break;
        }
    }
    if (best_format)
    {
        return *best_format;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Non-?RGB8888 formats not yet supported for display"}));
}
}

class mgc::CPUCopyOutputSurface::Impl
{
public:
    Impl(
        EGLDisplay dpy,
        EGLContext share_ctx,
        std::shared_ptr<mg::CPUAddressableDisplayProvider> allocator,
        geom::Size size);

    void bind();

    void make_current();
    void release_current();

    auto commit() -> std::unique_ptr<mg::Framebuffer>;

    auto size() const -> geom::Size;
    auto layout() const -> Layout;

private:
    std::shared_ptr<mg::CPUAddressableDisplayProvider> const allocator;
    EGLDisplay const dpy;
    EGLContext const ctx;
    geometry::Size const size_;
    DRMFormat const format;
    RenderbufferHandle const colour_buffer;
    FramebufferHandle const fbo;
};

mgc::CPUCopyOutputSurface::CPUCopyOutputSurface(
        EGLDisplay dpy,
        EGLContext share_ctx,
        std::shared_ptr<mg::CPUAddressableDisplayProvider> allocator,
        geom::Size size)
        : impl{std::make_unique<Impl>(dpy, share_ctx, std::move(allocator), size)}
{
}

mgc::CPUCopyOutputSurface::~CPUCopyOutputSurface() = default;

void mgc::CPUCopyOutputSurface::bind()
{
    impl->bind();
}

void mgc::CPUCopyOutputSurface::make_current()
{
    impl->make_current();
}

void mgc::CPUCopyOutputSurface::release_current()
{
    impl->make_current();
}

auto mgc::CPUCopyOutputSurface::commit() -> std::unique_ptr<mg::Framebuffer>
{
    return impl->commit();
}

auto mgc::CPUCopyOutputSurface::size() const -> geom::Size
{
    return impl->size();
}

auto mgc::CPUCopyOutputSurface::layout() const -> Layout
{
    return impl->layout();
}

mgc::CPUCopyOutputSurface::Impl::Impl(
    EGLDisplay dpy,
    EGLContext share_ctx,
    std::shared_ptr<mg::CPUAddressableDisplayProvider> allocator,
    geom::Size size)
    : allocator{std::move(allocator)},
      dpy{dpy},
      ctx{create_current_context(dpy, share_ctx)},
      size_{size},
      format{select_format_from(*this->allocator)}
{
    glBindRenderbuffer(GL_RENDERBUFFER, colour_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, size_.width.as_int(), size_.height.as_int());

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colour_buffer);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"}
                    ));
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                // Somehow we've managed to attach buffers with mismatched sizes?
                BOOST_THROW_EXCEPTION((
                    std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"}
                    ));
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                BOOST_THROW_EXCEPTION((
                    std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"}
                    ));
            case GL_FRAMEBUFFER_UNSUPPORTED:
                // This is the only one that isn't necessarily a programming error
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{"FBO is incomplete: formats selected are not supported by this GL driver"}
                    ));
            case 0:
                BOOST_THROW_EXCEPTION((
                    mg::gl_error("Failed to verify GL Framebuffer completeness")));
        }
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                std::string{"Unknown GL framebuffer error code: "} + std::to_string(status)}));
    }
}

void mgc::CPUCopyOutputSurface::Impl::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void mgc::CPUCopyOutputSurface::Impl::make_current()
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        mir::log_debug("Failed to make EGL context current");
    }
}

void mgc::CPUCopyOutputSurface::Impl::release_current()
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        mir::log_debug("Failed to release current EGL context");
    }
}

auto mgc::CPUCopyOutputSurface::Impl::commit() -> std::unique_ptr<mg::Framebuffer>
{
    auto fb = allocator->alloc_fb(size_, format);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    {
        /* TODO: We can usefully put this *into* DRMFormat */
        GLenum pixel_layout = GL_INVALID_ENUM;
        if (format == DRM_FORMAT_ARGB8888 || format == DRM_FORMAT_XRGB8888)
        {
            pixel_layout = GL_BGRA_EXT;
        }
        else if (format == DRM_FORMAT_RGBA8888 || format == DRM_FORMAT_RGBX8888)
        {
            pixel_layout = GL_RGBA;
        }
        auto mapping = fb->map_writeable();
        /*
         * TODO: This introduces a pipeline stall; GL must wait for all previous rendering commands
         * to complete before glReadPixels returns. We could instead do something fancy with
         * pixel buffer objects to defer this cost.
         */
        /*
         * TODO: We are assuming that the framebuffer pixel format is RGBX
         */
        glReadPixels(
            0, 0,
            size_.width.as_int(), size_.height.as_int(),
            pixel_layout, GL_UNSIGNED_BYTE, mapping->data());
    }
    return fb;
}

auto mgc::CPUCopyOutputSurface::Impl::size() const -> geom::Size
{
    return size_;
}

auto mgc::CPUCopyOutputSurface::Impl::layout() const -> Layout
{
    return Layout::TopRowFirst;
}
