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

#define MIR_LOG_COMPONENT "BlitterRenderer"

#include "framebuffer_pool.h"
#include "software_egl_context.h"

#include <mir/graphics/egl_dmabuf_import.h>
#include <mir/graphics/cpu_copy_output_surface.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/egl_helpers.h>
#include <mir/graphics/gl_config.h>
#include <mir/log.h>

#include <GLES2/gl2ext.h>
#include <boost/throw_exception.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mrb = mir::renderer::blitter;
namespace mrc = mir::renderer::common;

namespace
{
auto make_texture() -> mrc::TextureHandle
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    return mrc::TextureHandle{tex};
}

auto make_framebuffer() -> mrc::FramebufferHandle
{
    GLuint fb = 0;
    glGenFramebuffers(1, &fb);
    return mrc::FramebufferHandle{fb};
}

auto make_renderbuffer() -> mrb::RenderbufferHandle
{
    GLuint rb = 0;
    glGenRenderbuffers(1, &rb);
    return mrb::RenderbufferHandle{rb};
}

/// Capture the current EGL state for restoration on scope exit, unless \p context is already current
auto preserve_egl_state_unless_current(mrb::SoftwareEGLContext const& context) -> std::optional<mgc::CacheEglState>
{
    if (context.is_current())
    {
        return std::nullopt;
    }
    return mgc::CacheEglState{};
}
}

mrb::FramebufferPool::Entry::Entry(
    std::shared_ptr<SoftwareEGLContext> context,
    std::unique_ptr<mg::CPUAddressableDisplayAllocator::MappableFB> fb,
    std::unique_ptr<mg::BlitterRenderingProvider::Surface> surface,
    bool with_depth_stencil) :
    context{std::move(context)},
    fb{std::move(fb)},
    surface{std::move(surface)}
{
    auto const restore_egl_state = preserve_egl_state_unless_current(*this->context);
    this->context->make_current();

    try
    {
        build_gl_resources(with_depth_stencil);
    }
    catch (...)
    {
        release_gl_resources();
        throw;
    }
}

void mrb::FramebufferPool::Entry::build_gl_resources(bool with_depth_stencil)
{
    auto const* const dmabuf = fb->as_dmabuf();
    if (!dmabuf)
    {
        // TODO: Do a `glReadPixels` if dmabuf import fails
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Display framebuffer cannot be exported as a dma-buf; cannot use the blitter renderer"}));
    }

    auto const dpy = context->display();
    auto const& extensions = context->extensions();

    // The EGLImage is only needed to specify the texture's storage; once the texture
    // is an EGLImage sibling we can throw the image away without freeing the dma-buf.
    auto const image = mg::import_dmabuf_to_egl_image(dpy, extensions, *dmabuf);

    texture = make_texture();
    glBindTexture(GL_TEXTURE_2D, texture);
    extensions.base(dpy).glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    extensions.base(dpy).eglDestroyImageKHR(dpy, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    fbo = make_framebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (with_depth_stencil)
    {
        auto const buffer_size = size();
        depth_stencil_buffer = make_renderbuffer();
        glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, buffer_size.width.as_int(), buffer_size.height.as_int());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer);
    }

    if (auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            std::string{"Failed to bind display framebuffer as a GL render target: "} +
            (status == GL_FRAMEBUFFER_UNSUPPORTED ? "GL_FRAMEBUFFER_UNSUPPORTED" : std::to_string(status))}));
    }
}

mrb::FramebufferPool::Entry::~Entry()
{
    // We're about to release GL resources, so we need the context they belong
    // to to be current.
    auto const restore_egl_state = preserve_egl_state_unless_current(*context);
    try
    {
        context->make_current();
    }
    catch (...)
    {
        mir::log_warning("Failed to make EGL context current to release blitter framebuffer");
    }

    release_gl_resources();
}

void mrb::FramebufferPool::Entry::release_gl_resources()
{
    depth_stencil_buffer = RenderbufferHandle{};
    fbo = mrc::FramebufferHandle{};
    texture = mrc::TextureHandle{};
}

void mrb::FramebufferPool::Entry::bind()
{
    context->make_current();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

auto mrb::FramebufferPool::Entry::size() const -> geom::Size { return fb->size(); }

struct mrb::FramebufferPool::Self
{
    std::mutex free_entries_mutex;
    std::vector<std::unique_ptr<Entry>> free_entries;
    geometry::Size output_size;

    explicit Self(geometry::Size const& size) : output_size{size} {}

    void release(std::unique_ptr<Entry> entry)
    {
        std::lock_guard lock(free_entries_mutex);
        if (entry->size() == output_size)
        {
            free_entries.push_back(std::move(entry));
        }
    }
};

namespace
{
class PooledFBImpl : public mrb::FramebufferPool::PooledFB
{
public:
    PooledFBImpl(std::shared_ptr<mrb::FramebufferPool::Self> self, std::unique_ptr<mrb::FramebufferPool::Entry> entry) :
        self{std::move(self)},
        entry_{std::move(entry)}
    {}

    ~PooledFBImpl() override { self->release(std::move(entry_)); }

    auto size() const -> geom::Size override { return entry_->size(); }

    auto entry() -> mrb::FramebufferPool::Entry& override { return *entry_; }

private:
    std::shared_ptr<mrb::FramebufferPool::Self> const self;
    std::unique_ptr<mrb::FramebufferPool::Entry> entry_;
};
}

mrb::FramebufferPool::FramebufferPool(
    std::shared_ptr<SoftwareEGLContext> context,
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator,
    mg::GLConfig const& config) :
    context{std::move(context)},
    blitter{std::move(blitter)},
    allocator{allocator},
    format{mgc::select_format_from(allocator)},
    with_depth_stencil{config.depth_buffer_bits() || config.stencil_buffer_bits()},
    self{std::make_shared<Self>(allocator.output_size())}
{
    // Check we can actually build a usable framebuffer before anyone relies on us
    self->free_entries.push_back(make_entry());
}

mrb::FramebufferPool::~FramebufferPool() = default;

auto mrb::FramebufferPool::make_entry() -> std::unique_ptr<Entry>
{
    auto fb = allocator.alloc_fb(format);
    if (!fb)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to allocate display framebuffer for the blitter renderer"}));
    }

    auto surface = blitter->surface_for_fb(*fb);
    if (!surface)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Blitter cannot render into the display's framebuffers"}));
    }

    return std::make_unique<Entry>(context, std::move(fb), std::move(surface), with_depth_stencil);
}

auto mrb::FramebufferPool::acquire() -> std::unique_ptr<PooledFB>
{
    auto entry = [this]() -> std::unique_ptr<Entry>
    {
        std::lock_guard lock(self->free_entries_mutex);
        // Invalidate frame buffers of the old size if we need to.
        if (auto const current_size = allocator.output_size(); current_size != self->output_size)
        {
            self->output_size = current_size;
            self->free_entries.clear();
        }

        if (self->free_entries.empty())
        {
            return nullptr;
        }
        auto recycled = std::move(self->free_entries.back());
        self->free_entries.pop_back();
        return recycled;
    }();

    if (!entry)
    {
        entry = make_entry();
    }

    return std::make_unique<PooledFBImpl>(self, std::move(entry));
}
