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

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <mutex>
#include <stdexcept>
#include <cstring>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;

namespace
{

void set_guest_gbm_device(mg::NestedContext& nested_context, gbm_device* gbm_dev)
{
    MirMesaSetGBMDeviceRequest const request{gbm_dev};
    mg::PlatformOperationMessage request_msg;
    request_msg.data.resize(sizeof(MirMesaSetGBMDeviceRequest));
    std::memcpy(request_msg.data.data(), &request, sizeof(request));

    auto const response_msg = nested_context.platform_operation(
        MirMesaPlatformOperation::set_gbm_device,
        request_msg);

    if (response_msg.data.size() == sizeof(MirMesaSetGBMDeviceResponse))
    {
        static int const success{0};
        MirMesaSetGBMDeviceResponse response{-1};
        std::memcpy(&response, response_msg.data.data(), response_msg.data.size());
        if (response.status != success)
        {
            std::string const msg{"Nested Mir failed to set the gbm device."};
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(std::runtime_error(msg))
                    << boost::errinfo_errno(response.status));
        }
    }
    else
    {
        std::string const msg{"Nested Mir failed to set the gbm device: Invalid response."};
        BOOST_THROW_EXCEPTION(std::runtime_error(msg));
    }
}
}

mgm::GuestPlatform::GuestPlatform(
    std::shared_ptr<NestedContext> const& nested_context)
    : nested_context{nested_context}
{
    auto const fds = nested_context->platform_fd_items();
    gbm.setup(fds.at(0));
    set_guest_gbm_device(*nested_context, gbm.device);
    ipc_ops = std::make_shared<mgm::IpcOperations>(
        std::make_shared<mgm::NestedAuthentication>(nested_context));
}

std::shared_ptr<mg::GraphicBufferAllocator> mgm::GuestPlatform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::gbm_native_pixmap);
}

std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mg::NestedContext> const& nested_context)
{
    return std::make_shared<mgm::GuestPlatform>(nested_context);
}

std::shared_ptr<mg::PlatformIpcOperations> mgm::GuestPlatform::make_ipc_operations() const
{
    return ipc_ops;
}

std::shared_ptr<mg::Display> mgm::GuestPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
    std::shared_ptr<graphics::GLProgramFactory> const&,
    std::shared_ptr<graphics::GLConfig> const& /*gl_config*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("mgm::GuestPlatform cannot create display\n"));
}

EGLNativeDisplayType mgm::GuestPlatform::egl_native_display() const
{
    return gbm.device;
}
