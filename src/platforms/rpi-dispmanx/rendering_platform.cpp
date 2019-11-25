/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/module_deleter.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"

#include "rendering_platform.h"

#include "buffer_allocator.h"

namespace mg = mir::graphics;

auto mg::rpi::RenderingPlatform::create_buffer_allocator(Display const &output)
  ->mir::UniqueModulePtr<GraphicBufferAllocator>
{
    return mir::make_module_ptr<rpi::BufferAllocator>(output);
}

auto mir::graphics::rpi::RenderingPlatform::make_ipc_operations() const
  -> mir::UniqueModulePtr<PlatformIpcOperations>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"rpi-dispmanx platform does not support mirclient"}));
}

auto mir::graphics::rpi::RenderingPlatform::native_rendering_platform()
  -> mir::graphics::NativeRenderingPlatform *
{
    return nullptr;
}
