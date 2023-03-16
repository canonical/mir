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
#include "buffer_allocator.h"

#define MIR_LOG_COMPONENT "platform-graphics-gbm-kms"
#include "mir/log.h"

#include <wayland-server-protocol.h>

namespace mg = mir::graphics;
namespace mge = mg::egl::generic;

mge::RenderingPlatform::RenderingPlatform()
{
}

auto mge::RenderingPlatform::create_buffer_allocator(
    mg::Display const& output) -> mir::UniqueModulePtr<mg::GraphicBufferAllocator>
{
    return make_module_ptr<mge::BufferAllocator>(output);
}
