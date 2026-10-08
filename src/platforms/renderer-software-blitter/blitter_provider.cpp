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

#include "blitter_provider.h"

#include <mir/graphics/display_providers.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/egl_context_executor.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/egl_helpers.h>
#include <mir/graphics/gl_format.h>
#include <mir/graphics/pixel_format_utils.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/gl/gl_surface.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/renderers/gl/renderer.h>
#include <mir_toolkit/common.h>

#include <boost/throw_exception.hpp>
#include <GL/gl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <array>
#include <cstring>
#include <utility>

namespace mg = mir::graphics;
namespace mgc = mg::common;
namespace mgsb = mg::software_blitter;
namespace mge = mg::egl::generic;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

namespace
{
template<void (*allocator)(GLsizei, GLuint*), void (*deleter)(GLsizei, GLuint const*)>
class GLHandle
{
public:
    GLHandle() { (*allocator)(1, &id); }

    ~GLHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;

    void reset()
    {
        if (id)
            (*deleter)(1, &id);
        id = 0;
    }

    GLHandle(GLHandle&& from) : id{from.id} { from.id = 0; }

    operator GLuint() const { return id; }

private:
    GLuint id{0};
};

using TexturebufferHandle = GLHandle<&glGenTextures, &glDeleteTextures>;
using FramebufferHandle = GLHandle<&glGenFramebuffers, &glDeleteFramebuffers>;

auto create_current_context(EGLDisplay dpy, EGLContext share_ctx) -> EGLContext
{
    static EGLint const context_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    auto const egl_extensions = eglQueryString(dpy, EGL_EXTENSIONS);
    if (std::strstr(egl_extensions, "EGL_KHR_no_config_context") == nullptr)
    {
        // We do not *strictly* need this, but it means I don't need to thread a GLConfig all the way through to here.
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "EGL implementation missing necessary EGL_KHR_no_config_context extension"}));
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    auto ctx = eglCreateContext(dpy, EGL_NO_CONFIG_KHR, share_ctx, context_attr);

    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
    }
    return ctx;
}

class MappableFBOutputSurface : public mg::gl::OutputSurface
{
public:
    explicit MappableFBOutputSurface(
        std::shared_ptr<mg::DMABufBuffer> dmabuf,
        std::shared_ptr<mg::DMABufEGLProvider> const& dmabuf_provider,
        EGLDisplay dpy,
        EGLContext ctx) :
        dpy_{dpy},
        ctx_{create_current_context(dpy, ctx)},
        size_{dmabuf->size()},
        texture_{dmabuf_provider->as_texture(std::move(dmabuf))}
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_->tex_id(), 0);

        auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            switch (status)
            {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                BOOST_THROW_EXCEPTION((std::runtime_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"}));
            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                // Somehow we've managed to attach buffers with mismatched sizes?
                BOOST_THROW_EXCEPTION((std::logic_error{"FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"}));
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                BOOST_THROW_EXCEPTION((std::logic_error{
                    "FBO is incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"}));
            case GL_FRAMEBUFFER_UNSUPPORTED:
                // This is the only one that isn't necessarily a programming error
                BOOST_THROW_EXCEPTION((std::runtime_error{
                    "FBO is incomplete: formats selected are not supported by this GL driver"}));
            case 0:
                BOOST_THROW_EXCEPTION((mg::gl_error("Failed to verify GL Framebuffer completeness")));
            default:
                BOOST_THROW_EXCEPTION((std::runtime_error{
                    std::string{"Unknown GL framebuffer error code: "} + std::to_string(status)}));
            }
        }
    }

    ~MappableFBOutputSurface() override
    {
        // Capture current EGL state to restore, if the current context is not the one we're destroying
        auto const egl_restore = [this]() -> std::optional<mgc::CacheEglState>
        {
            if (ctx_ != eglGetCurrentContext())
            {
                // We're not current; capture the current state...
                auto current_state = mgc::CacheEglState{};
                // ...then *make* us current, so we can release our resources
                make_current();
                return current_state;
            }

            // We *are* the current context; we don't need to restore EGL state
            return std::nullopt;
        }();

        // We're the current EGL context; destroy our GL resources...
        fbo_.reset();
        texture_.reset();

        // Now release our context, and delete it.
        release_current();
        eglDestroyContext(dpy_, ctx_);
    }

    void bind() override { glBindFramebuffer(GL_FRAMEBUFFER, fbo_); }
    void make_current() override { eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx_); }
    void release_current() override { eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }
    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        glFinish();
        // In `wait_complete`, the result of `renderer.render` (this framebuffer) is unsued. So we
        // can safely return `nullptr` knowing that it will not be used anywhere.
        return nullptr;
    }
    auto size() const -> geom::Size override { return size_; }
    auto layout() const -> Layout override { return Layout::TopRowFirst; }

private:
    EGLDisplay const dpy_;
    EGLContext const ctx_;
    FramebufferHandle fbo_;
    geom::Size const size_;
    std::shared_ptr<mg::gl::Texture> texture_;
};
}

class mg::BlitterRenderingProvider::Surface
{
public:
    explicit Surface(std::shared_ptr<CPUAddressableDisplayAllocator::MappableFB> fb) : fb_{std::move(fb)} {};

    auto create_output_surface(
        EGLDisplay dpy,
        EGLContext ctx,
        std::shared_ptr<mg::DMABufEGLProvider> const& dmabuf_provider) -> std::unique_ptr<MappableFBOutputSurface>
    {
        // TODO: Casting `const` away, not good.
        std::shared_ptr<DMABufBuffer> dmabuf{
            fb_, const_cast<DMABufBuffer*>(fb_->as_dmabuf())}; // TICS -cppcoreguidelines-pro-type-const-cast
        auto surface = std::make_unique<MappableFBOutputSurface>(dmabuf, dmabuf_provider, dpy, ctx);
        return surface;
    }

private:
    std::shared_ptr<CPUAddressableDisplayAllocator::MappableFB> fb_;
};

class mg::BlitterRenderingProvider::Task
{
public:
    explicit Task(std::unique_ptr<Surface> surface) : surface{std::move(surface)} {}
    ~Task() = default;

    auto take_surface() -> std::unique_ptr<Surface> { return std::move(surface); }
    auto take_renderables() -> mg::RenderableList { return std::move(renderables); }

    void push(std::shared_ptr<mg::Renderable> renderable) { renderables.push_back(std::move(renderable)); }

private:
    std::unique_ptr<Surface> surface;
    mg::RenderableList renderables;
};

namespace
{
class FillRenderable : public mg::Renderable
{
public:
    FillRenderable(
        mg::GraphicBufferAllocator& allocator,
        mir::geometry::Rectangle target_rect,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a) :
        target_rect{target_rect},
        alpha_{a},
        buffer_{mrs::alloc_buffer_with_content(
            allocator,
            std::array<unsigned char, 4>{r, g, b, a}.data(),
            {1, 1},
            geom::Stride{4},
            mir_pixel_format_abgr_8888)}
    {}

    auto id() const -> mg::Renderable::ID override { return this; }

    auto buffer() const -> std::shared_ptr<mg::Buffer> override { return buffer_; }

    auto screen_position() const -> geom::Rectangle override { return target_rect; }

    auto src_bounds() const -> geom::RectangleD override { return {{0, 0}, buffer_->size()}; }

    auto clip_area() const -> std::optional<geom::Rectangle> override { return {}; }

    auto alpha() const -> float override { return static_cast<float>(alpha_) / 255.0f; }

    auto transformation() const -> glm::mat4 override { return glm::mat4{1}; }

    auto orientation() const -> MirOrientation override { return mir_orientation_normal; }

    auto mirror_mode() const -> MirMirrorMode override { return mir_mirror_mode_none; }

    auto shaped() const -> bool override { return false; }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override { return std::nullopt; }

    auto opaque_region() const -> std::optional<geom::Rectangles> override { return std::nullopt; }

private:
    mir::geometry::Rectangle const target_rect;
    uint8_t alpha_;
    std::shared_ptr<mg::Buffer> const buffer_;
};

class BlitRenderable : public mg::Renderable
{
public:
    BlitRenderable(
        std::shared_ptr<mg::Buffer> source,
        mir::geometry::Rectangle source_rect,
        mir::geometry::Rectangle target_rect,
        MirOrientation orientation,
        MirMirrorMode mirror_mode) :
        source{source},
        source_rect{source_rect},
        target_rect{target_rect},
        orientation_{orientation},
        mirror_mode_{mirror_mode}
    {}

    auto id() const -> mg::Renderable::ID override { return this; }

    auto buffer() const -> std::shared_ptr<mg::Buffer> override { return source; }

    auto screen_position() const -> geom::Rectangle override { return target_rect; }

    auto src_bounds() const -> geom::RectangleD override
    {
        return {geom::PointD{source_rect.top_left}, geom::SizeD{source_rect.size}};
    }

    auto clip_area() const -> std::optional<geom::Rectangle> override { return {}; }

    auto alpha() const -> float override { return 1.0f; }

    auto transformation() const -> glm::mat4 override { return glm::mat4{1}; }

    auto orientation() const -> MirOrientation override { return orientation_; }

    auto mirror_mode() const -> MirMirrorMode override { return mirror_mode_; }

    auto shaped() const -> bool override { return mg::contains_alpha(source->pixel_format()); }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override { return std::nullopt; }

    auto opaque_region() const -> std::optional<geom::Rectangles> override { return std::nullopt; }

private:
    std::shared_ptr<mg::Buffer> source;
    mir::geometry::Rectangle source_rect;
    mir::geometry::Rectangle target_rect;
    MirOrientation orientation_;
    MirMirrorMode mirror_mode_;
};

}

mgsb::SoftwareBlitterRenderingProvider::SoftwareBlitterRenderingProvider(
    EGLDisplay dpy,
    EGLContext ctx,
    std::unique_ptr<mge::BufferAllocator> allocator,
    std::shared_ptr<mg::DMABufEGLProvider> dmabuf_provider,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate) :
    mg::BlitterRenderingProvider(),
    allocator{std::move(allocator)},
    gl_rendering_provider{std::make_shared<mge::GLRenderingProvider>(dpy, ctx, dmabuf_provider, egl_delegate)},
    dpy{dpy},
    ctx{ctx},
    dmabuf_provider{std::move(dmabuf_provider)}
{}

auto mgsb::SoftwareBlitterRenderingProvider::create_task(std::unique_ptr<Surface> surface) -> std::unique_ptr<Task>
{
    return std::make_unique<Task>(std::move(surface));
}

auto mgsb::SoftwareBlitterRenderingProvider::wait_complete(std::unique_ptr<Task> task) -> std::unique_ptr<Surface>
{
    auto surface = task->take_surface();
    auto output_surface = surface->create_output_surface(dpy, ctx, dmabuf_provider);
    auto const size = output_surface->size();
    mir::renderer::gl::Renderer renderer{gl_rendering_provider, std::move(output_surface)};
    renderer.set_viewport({{0, 0}, size});
    renderer.set_output_transform(glm::mat2{1.0f});
    renderer.render(task->take_renderables());
    return surface;
}

auto mgsb::SoftwareBlitterRenderingProvider::blit(
    Task& task,
    std::shared_ptr<Buffer> const& source,
    geometry::Rectangle const& source_rect,
    geometry::Rectangle const& target_rect,
    MirOrientation rotation,
    MirMirrorMode mirror_mode) -> bool
{
    if (!gl_rendering_provider->as_texture(source))
    {
        return false;
    }

    task.push(std::make_shared<BlitRenderable>(source, source_rect, target_rect, rotation, mirror_mode));
    return true;
}

auto mgsb::SoftwareBlitterRenderingProvider::fill(
    Task& task,
    geometry::Rectangle const& target_rect,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a) -> bool
{
    task.push(std::make_shared<FillRenderable>(*allocator, target_rect, r, g, b, a));
    return true;
}

auto mgsb::SoftwareBlitterRenderingProvider::surface_for_fb(
    std::shared_ptr<CPUAddressableDisplayAllocator::MappableFB> const& fb) -> std::unique_ptr<Surface>
{
    return std::make_unique<Surface>(fb);
}

auto mgsb::SoftwareBlitterRenderingProvider::suitability_for_display(DisplaySink& sink) -> probe::Result
{
    return gl_rendering_provider->suitability_for_display(sink);
}

auto mgsb::SoftwareBlitterRenderingProvider::suitability_for_allocator(
    std::shared_ptr<GraphicBufferAllocator> const& target) -> probe::Result
{
    return gl_rendering_provider->suitability_for_allocator(target);
}

auto mgsb::SoftwareBlitterRenderingProvider::make_framebuffer_provider(DisplaySink& sink)
    -> std::unique_ptr<FramebufferProvider>
{
    return gl_rendering_provider->make_framebuffer_provider(sink);
}
