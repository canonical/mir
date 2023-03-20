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

#ifndef MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_
#define MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_

#include "mir/graphics/platform.h"

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace graphics::egl::generic
{

class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    explicit RenderingPlatform();

    auto create_buffer_allocator(
        graphics::Display const& output) -> UniqueModulePtr<graphics::GraphicBufferAllocator> override;
};

}
}
#endif /* MIR_GRAPHICS_RENDERING_EGL_GENERIC_H_ */
