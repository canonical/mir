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
#include "mesa_extensions.h"

#include "mir/graphics/platform_authentication.h"
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
void set_guest_gbm_device(mg::PlatformAuthentication& platform_authentication, gbm_device* device)
{
    std::string const msg{"Nested Mir failed to set the gbm device."};
    auto ext = platform_authentication.set_gbm_extension();
    if (ext.is_set())
        ext.value()->set_gbm_device(device);
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir failed to set the gbm device."));
}
}

mgm::GuestPlatform::NestedAuthFactory::NestedAuthFactory(
    std::shared_ptr<mg::PlatformAuthentication> const& auth) :
    auth(auth)
{
}

mir::UniqueModulePtr<mg::PlatformAuthentication>
mgm::GuestPlatform::NestedAuthFactory::create_platform_authentication()
{
    struct AuthenticationWrapper : mg::PlatformAuthentication
    {
        AuthenticationWrapper(std::shared_ptr<PlatformAuthentication> const& auth) :
            auth(auth)
        {
        }

        mir::optional_value<std::shared_ptr<mg::MesaAuthExtension>> auth_extension() override
        {
            return auth->auth_extension();
        }

        mir::optional_value<std::shared_ptr<mg::SetGbmExtension>> set_gbm_extension() override
        {
            return auth->set_gbm_extension();
        }

        mg::PlatformOperationMessage platform_operation(
            unsigned int op, mg::PlatformOperationMessage const& msg) override
        {
            return auth->platform_operation(op, msg);
        }
        mir::optional_value<mir::Fd> drm_fd() override
        {
            return auth->drm_fd();
        }
        std::shared_ptr<mg::PlatformAuthentication> const auth;
    };
    return make_module_ptr<AuthenticationWrapper>(auth);
}

mgm::GuestPlatform::GuestPlatform(
    std::shared_ptr<mg::PlatformAuthentication> const& platform_authentication) :
    platform_authentication{platform_authentication},
    auth_factory{platform_authentication}
{
    auto ext = platform_authentication->auth_extension();
    if (!ext.is_set())
        BOOST_THROW_EXCEPTION(std::runtime_error("could not access drm auth fd"));
    gbm.setup(ext.value()->auth_fd());
    set_guest_gbm_device(*platform_authentication, gbm.device);
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgm::GuestPlatform::create_buffer_allocator()
{
    return mir::make_module_ptr<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::gbm_native_pixmap);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgm::GuestPlatform::make_ipc_operations() const
{
    return mir::make_module_ptr<mgm::IpcOperations>(
        std::make_shared<mgm::NestedAuthentication>(platform_authentication));
}

mir::UniqueModulePtr<mg::Display> mgm::GuestPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
    std::shared_ptr<graphics::GLConfig> const& /*gl_config*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("mgm::GuestPlatform cannot create display\n"));
}

mg::NativeDisplayPlatform* mgm::GuestPlatform::native_display_platform()
{
    return &auth_factory;
}

mg::NativeRenderingPlatform* mgm::GuestPlatform::native_rendering_platform()
{
    return this;
}

MirEGLNativeDisplayType mgm::GuestPlatform::egl_native_display() const
{
    return gbm.device;
}

std::vector<mir::ExtensionDescription> mgm::GuestPlatform::extensions() const
{
    return mgm::mesa_extensions();
}
