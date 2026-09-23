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

#include "rendering_platform.h"

#include <mir/graphics/rendering_providers.h>

#include "blitter_provider.h"

namespace mg = mir::graphics;
namespace mge = mg::egl::generic;
namespace mgsb = mg::software_blitter;

auto mgsb::SoftwareBlitterRenderingPlatform::maybe_create_provider(mg::RenderingProvider::Tag const& type_tag)
    -> std::shared_ptr<mg::RenderingProvider>
{
    if (dynamic_cast<mg::BlitterRenderingProvider::Tag const*>(&type_tag))
    {
        auto const dpy = egl_display();
        auto const ctx = egl_context();
        auto const dmabuf_provider = dma_buf_provider();
        return std::make_shared<mgsb::SoftwareBlitterRenderingProvider>(
            dpy,
            ctx,
            make_module_ptr<mge::BufferAllocator>(dpy, ctx, dmabuf_provider),
            dmabuf_provider,
            egl_context_executor());
    }
    return nullptr;
}
