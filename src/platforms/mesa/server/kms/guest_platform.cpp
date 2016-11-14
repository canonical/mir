/*
 * Copyright © 2013 Canonical Ltd.
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
 * Eleni Maria Stea <elenimaria.stea@canonical.com>
 * Alan Griffiths <alan@octopull.co.uk>
 */

#include "guest_platform.h"
#include "nested_authentication.h"
#include "ipc_operations.h"
#include "buffer_allocator.h"

#include "mir/graphics/nested_context.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "mir_toolkit/extensions/set_gbm_device.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <mutex>
#include <stdexcept>
#include <cstring>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;

namespace
{
//TODO: construction for mclm::ClientPlatform is roundabout/2-step.
//      Might be better for the extension to be a different way to allocate
//      MirConnection, but beyond scope of work.
void set_guest_gbm_device(mg::NestedContext& nested_context, gbm_device* device)
{
    std::string const msg{"Nested Mir failed to set the gbm device."};
    auto ext = static_cast<MirExtensionSetGbmDevice*>(nested_context.request_interface(
        MIR_EXTENSION_SET_GBM_DEVICE, MIR_EXTENSION_SET_GBM_DEVICE_VERSION_1));
    if (!ext || !ext->set_gbm_device)
        BOOST_THROW_EXCEPTION(std::runtime_error(msg));
    ext->set_gbm_device(device, ext->context);
}
}

mgm::GuestPlatform::GuestPlatform(
    std::shared_ptr<NestedContext> const& nested_context)
    : nested_context{nested_context}
{
    auto const fds = nested_context->platform_fd_items();
    gbm.setup(fds.at(0));
    set_guest_gbm_device(*nested_context, gbm.device);
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgm::GuestPlatform::create_buffer_allocator()
{
    return mir::make_module_ptr<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::gbm_native_pixmap);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgm::GuestPlatform::make_ipc_operations() const
{
    return mir::make_module_ptr<mgm::IpcOperations>(
        std::make_shared<mgm::NestedAuthentication>(nested_context));
}

mir::UniqueModulePtr<mg::Display> mgm::GuestPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
    std::shared_ptr<graphics::GLConfig> const& /*gl_config*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("mgm::GuestPlatform cannot create display\n"));
}
