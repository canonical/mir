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
 * Authored by:
 * Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "guest_platform.h"
#include "ipc_operations.h"
#include "buffer_allocator.h"

#include "mir/graphics/nested_context.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace mgm = mg::mesa;

mgx::GuestPlatform::GuestPlatform(
    std::shared_ptr<NestedContext> const& /*nested_context*/)
    : udev{std::make_shared<mir::udev::Context>()},
      drm{std::make_shared<mesa::helpers::DRMHelper>(mesa::helpers::DRMNodeToUse::render)}
{
    drm->setup(udev);
    gbm.setup(*drm);
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::GuestPlatform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(
               gbm.device,
               mgm::BypassOption::prohibited,
               mgm::BufferImportMethod::dma_buf);
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::GuestPlatform::make_ipc_operations() const
{
    return std::make_shared<mg::mesa::IpcOperations>(drm);
}

std::shared_ptr<mg::Display> mgx::GuestPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
    std::shared_ptr<graphics::GLProgramFactory> const&,
    std::shared_ptr<graphics::GLConfig> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Guest platform cannot create display\n"));
}

EGLNativeDisplayType mgx::GuestPlatform::egl_native_display() const
{
    return gbm.device;
}
