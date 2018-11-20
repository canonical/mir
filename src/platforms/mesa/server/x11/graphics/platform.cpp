/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "platform.h"
#include "display.h"
#include "buffer_allocator.h"
#include "ipc_operations.h"
#include "mesa_extensions.h"

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgx = mg::X;
namespace geom = mir::geometry;

mgx::Platform::Platform(std::shared_ptr<::Display> const& conn,
                        geom::Size const size,
                        std::shared_ptr<mg::DisplayReport> const& report)
    : x11_connection{conn},
      udev{std::make_shared<mir::udev::Context>()},
      drm{mgm::helpers::DRMHelper::open_any_render_node(udev)},
      report{report},
      gbm{drm->fd},
      size{size}
{
    if (!x11_connection)
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 display"));

    auth_factory = std::make_unique<mgm::DRMNativePlatformAuthFactory>(*drm);
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator(
    mg::Display const& output)
{
    return make_module_ptr<mgm::BufferAllocator>(output, gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::dma_buf);
}

mir::UniqueModulePtr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return make_module_ptr<mgx::Display>(x11_connection.get(), size, gl_config,
                                         report);
}

mg::NativeDisplayPlatform* mgx::Platform::native_display_platform()
{
    return auth_factory.get();
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    return make_module_ptr<mg::mesa::IpcOperations>(drm);
}

mg::NativeRenderingPlatform* mgx::Platform::native_rendering_platform()
{
    return this;
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
    return eglGetDisplay(x11_connection.get());
}

std::vector<mir::ExtensionDescription> mgx::Platform::extensions() const
{
    return mgm::mesa_extensions();
}
