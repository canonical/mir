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
#include "egl_context.h"

#include "graphics/display_format_selection.h"
#include "graphics/egl_dmabuf_import.h"

#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/gl_config.h>
#include <mir/log.h>

#include <GLES2/gl2ext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <utility>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;

namespace
{
auto make_texture() -> mir::renderer::common::TextureHandle
{
    GLuint tex{0};
    glGenTextures(1, &tex);
    return mir::renderer::common::TextureHandle{tex};
}

auto make_framebuffer() -> mir::renderer::common::FramebufferHandle
{
    GLuint fb{0};
    glGenFramebuffers(1, &fb);
    return mir::renderer::common::FramebufferHandle{fb};
}

auto make_renderbuffer() -> mrb::RenderbufferHandle
{
    GLuint rb{0};
    glGenRenderbuffers(1, &rb);
    return mrb::RenderbufferHandle{rb};
}
}

mrb::FramebufferPool::Entry::Entry(
    std::shared_ptr<SoftwareEGLContext> context,
    std::unique_ptr<mg::CPUAddressableDisplayAllocator::MappableFB> fb,
    std::unique_ptr<mg::BlitterRenderingProvider::Surface> surface,
    bool with_depth_stencil)
    : surface{std::move(surface)},
      context{std::move(context)},
      fb{std::move(fb)}
{
    this->context->make_current();

    auto const* const dmabuf = this->fb->as_dmabuf();
    if (!dmabuf)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Display framebuffer cannot be exported as a dma-buf; cannot use the blitter renderer"}));
    }

    auto const dpy = this->context->display();
    auto const& extensions = this->context->extensions();

    /* The EGLImage is only needed to specify the texture's storage; once the texture
     * is an EGLImage sibling we can throw the image away without freeing the dma-buf.
     */
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
            GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
            buffer_size.width.as_int(), buffer_size.height.as_int());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer);
    }

    if (auto const status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status != GL_FRAMEBUFFER_COMPLETE)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            std::string{"Failed to bind display framebuffer as a GL render target: "} +
            (status == GL_FRAMEBUFFER_UNSUPPORTED ?
                "GL_FRAMEBUFFER_UNSUPPORTED" : std::to_string(status))}));
    }
}

mrb::FramebufferPool::Entry::~Entry()
{
    /* We're about to release GL resources, so we need the context they belong
     * to to be current.
     */
    try
    {
        context->make_current();
    }
    catch (...)
    {
        mir::log_warning("Failed to make EGL context current to release blitter framebuffer");
    }
}

void mrb::FramebufferPool::Entry::bind_gl()
{
    context->make_current();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

auto mrb::FramebufferPool::Entry::size() const -> geom::Size
{
    return fb->size();
}

class mrb::FramebufferPool::PooledFB : public mg::Framebuffer
{
public:
    PooledFB(std::shared_ptr<FramebufferPool> pool, std::unique_ptr<Entry> entry)
        : pool{std::move(pool)},
          entry{std::move(entry)}
    {
    }

    ~PooledFB() override
    {
        pool->release(std::move(entry));
    }

    auto size() const -> geom::Size override
    {
        return entry->size();
    }

private:
    std::shared_ptr<FramebufferPool> const pool;
    std::unique_ptr<Entry> entry;
};

mrb::FramebufferPool::FramebufferPool(
    std::shared_ptr<SoftwareEGLContext> context,
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator,
    mg::DRMFormat format,
    bool with_depth_stencil)
    : context{std::move(context)},
      blitter{std::move(blitter)},
      allocator{allocator},
      format{format},
      with_depth_stencil{with_depth_stencil},
      output_size{allocator.output_size()}
{
}

mrb::FramebufferPool::~FramebufferPool() = default;

auto mrb::FramebufferPool::create(
    std::shared_ptr<SoftwareEGLContext> context,
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator,
    mg::GLConfig const& config) -> std::shared_ptr<FramebufferPool>
{
    std::shared_ptr<FramebufferPool> pool{
        new FramebufferPool{
            std::move(context),
            std::move(blitter),
            allocator,
            mg::select_format_from(allocator),
            config.depth_buffer_bits() || config.stencil_buffer_bits()}};

    // Check we can actually build a usable framebuffer before anyone relies on us
    pool->free_entries.push_back(pool->make_entry());

    return pool;
}

auto mrb::FramebufferPool::make_entry() -> std::unique_ptr<Entry>
{
    auto fb = allocator.alloc_fb(format);
    auto surface = blitter->surface_for_fb(*fb);
    if (!surface)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Blitter cannot render into the display's framebuffers"}));
    }

    return std::make_unique<Entry>(context, std::move(fb), std::move(surface), with_depth_stencil);
}

auto mrb::FramebufferPool::acquire() -> Checkout
{
    auto entry =
        [this]() -> std::unique_ptr<Entry>
        {
            std::lock_guard lock{free_entries_mutex};
            if (free_entries.empty())
            {
                return nullptr;
            }
            auto recycled = std::move(free_entries.back());
            free_entries.pop_back();
            return recycled;
        }();

    if (!entry)
    {
        entry = make_entry();
    }

    auto& entry_ref = *entry;
    return Checkout{entry_ref, std::make_unique<PooledFB>(shared_from_this(), std::move(entry))};
}

void mrb::FramebufferPool::release(std::unique_ptr<Entry> entry)
{
    std::lock_guard lock{free_entries_mutex};
    free_entries.push_back(std::move(entry));
}

auto mrb::FramebufferPool::size() const -> geom::Size
{
    return output_size;
}
