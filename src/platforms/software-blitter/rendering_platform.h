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

#ifndef MIR_GRAPHICS_RENDERING_SOFTWARE_BLITTER_H
#define MIR_GRAPHICS_RENDERING_SOFTWARE_BLITTER_H

#include <mir/graphics/platform.h>

namespace mir
{
namespace graphics
{
namespace software_blitter
{

class SoftwareBlitterRenderingPlatform : public graphics::RenderingPlatform
{
public:
    explicit SoftwareBlitterRenderingPlatform();

    ~SoftwareBlitterRenderingPlatform();

    auto create_buffer_allocator(graphics::Display const& output)
        -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;

protected:
    auto maybe_create_provider(RenderingProvider::Tag const& type_tag) -> std::shared_ptr<RenderingProvider> override;
};

}
}
}

#endif // MIR_GRAPHICS_RENDERING_SOFTWARE_BLITTER_H
