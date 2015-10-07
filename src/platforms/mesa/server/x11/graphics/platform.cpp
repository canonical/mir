/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgx = mg::X;
namespace geom = mir::geometry;

mgx::Platform::Platform(std::shared_ptr<::Display> const& conn, geom::Size const size)
    : x11_connection{conn},
      udev{std::make_shared<mir::udev::Context>()},
      drm{std::make_shared<mesa::helpers::DRMHelper>(mesa::helpers::DRMNodeToUse::render)},
      size{size}
{
    if (!x11_connection)
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 display"));

    drm->setup(udev);
    gbm.setup(*drm);
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::dma_buf);
}

std::shared_ptr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
    return std::make_shared<mgx::Display>(x11_connection.get(), size);
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    return std::make_shared<mg::mesa::IpcOperations>(drm);
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
    return eglGetDisplay(x11_connection.get());
}
