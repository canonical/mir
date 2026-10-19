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
#include <mir/log.h>
#include <mir/synchronised.h>

#include <boost/throw_exception.hpp>
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
}

mrb::FramebufferPool::Entry::Entry(
    std::shared_ptr<SoftwareEGLContext> context,
    std::unique_ptr<mg::CPUAddressableDisplayAllocator::MappableFB> fb,
    std::unique_ptr<mg::BlitterRenderingProvider::Surface> surface) :
    context{std::move(context)},
    fb{std::move(fb)},
    surface{std::move(surface)}
{
    auto const restore_egl_state = preserve_egl_state_unless_current(*this->context);
    this->context->make_current();

    try
    {
        build_gl_resources();
    }
    catch (...)
    {
        release_gl_resources();
        throw;
    }
}

void mrb::FramebufferPool::Entry::build_gl_resources()
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
    fbo = mrc::FramebufferHandle{};
    texture = mrc::TextureHandle{};
}

void mrb::FramebufferPool::Entry::bind()
{
    context->make_current();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

auto mrb::FramebufferPool::Entry::size() const -> geom::Size { return fb->size(); }

auto mrb::FramebufferPool::Entry::framebuffer() -> mg::CPUAddressableDisplayAllocator::MappableFB&
{
    return *fb;
}

class mrb::FramebufferPool::Self
{

public:
    explicit Self(geometry::Size const& size) : state{State{{}, size}} {}

    void add(std::unique_ptr<Entry> entry)
    {
        state.lock()->free_entries.push_back(std::move(entry));
    }

    void release(std::unique_ptr<Entry> entry)
    {
        auto const locked = state.lock();
        if (entry->size() == locked->output_size)
        {
            locked->free_entries.push_back(std::move(entry));
        }
    }

    auto acquire(mg::CPUAddressableDisplayAllocator& allocator) -> std::unique_ptr<Entry>
    {
        auto const locked = state.lock();
        // Invalidate frame buffers of the old size if we need to.
        if (auto const current_size = allocator.output_size(); current_size != locked->output_size)
        {
            locked->output_size = current_size;
            locked->free_entries.clear();
        }

        if (locked->free_entries.empty())
        {
            return nullptr;
        }
        auto recycled = std::move(locked->free_entries.back());
        locked->free_entries.pop_back();
        return recycled;
    }

private:
    struct State
    {
        std::vector<std::unique_ptr<Entry>> free_entries;
        geometry::Size output_size;
    };
    mir::Synchronised<State> state;
};

mrb::FramebufferPool::FramebufferPool(
    std::shared_ptr<SoftwareEGLContext> context,
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator) :
    context{std::move(context)},
    blitter{std::move(blitter)},
    allocator{allocator},
    format{mgc::select_format_from(allocator)},
    self{std::make_shared<Self>(allocator.output_size())}
{
    // Check we can actually build a usable framebuffer before anyone relies on us
    self->add(make_entry());
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

    return std::make_unique<Entry>(context, std::move(fb), std::move(surface));
}

auto mrb::FramebufferPool::acquire() -> PooledFB
{
    auto entry = self->acquire(allocator);
    if (!entry)
    {
        entry = make_entry();
    }

    auto* const checked_out = entry.get();
    // The entry keeps owning the framebuffer; the deleter just holds the entry until it's dropped
    mg::FramebufferPtr framebuffer{
        &checked_out->framebuffer(),
        mg::FramebufferDeleter{
            [self = self, entry = std::move(entry)](mg::Framebuffer*) mutable { self->release(std::move(entry)); }}};

    return {checked_out, std::move(framebuffer)};
}
