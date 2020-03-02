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
 */

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "buffer_allocator.h"
#include "mir/fatal.h"

#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
using namespace std::literals;

mgw::Platform::Platform(struct wl_display* const wl_display, std::shared_ptr<mg::DisplayReport> const& report) :
    wl_display{wl_display},
    report{report}
{
    if (!wl_display)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to connect to wayland"));
    }
}

mir::UniqueModulePtr<mg::Display> mgw::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& policy,
    std::shared_ptr<GLConfig> const& gl_config)
{
  return mir::make_module_ptr<mgw::Display>(wl_display, gl_config, policy, report);
}

mg::NativeDisplayPlatform* mgw::Platform::native_display_platform()
{
    return nullptr;
}

EGLNativeDisplayType mgw::Platform::egl_native_display() const
{
    return eglGetDisplay(wl_display);
}


std::vector<mir::ExtensionDescription> mgw::Platform::extensions() const
{
    fatal_error("wayland platform does not support mirclient");
    return {};
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgw::Platform::create_buffer_allocator(mg::Display const& output)
{
    return mir::make_module_ptr<mgw::BufferAllocator>(output);
}

mg::NativeRenderingPlatform* mgw::Platform::native_rendering_platform()
{
    return this;
}
mir::UniqueModulePtr<mg::PlatformIpcOperations> mgw::Platform::make_ipc_operations() const
{
    fatal_error("wayland platform does not support mirclient");
    return {};
}
