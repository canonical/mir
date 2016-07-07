/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/graphics/buffer_ipc_message.h"

#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <xf86drm.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mge = mir::graphics::eglstream;

namespace
{
// Our copy of eglext.h doesn't have this?
int const EGL_DRM_MASTER_FD_EXT{0x333C};

char const* drm_node_for_device(EGLDeviceEXT device)
{
    auto const device_path = eglQueryDeviceStringEXT(device, EGL_DRM_DEVICE_FILE_EXT);
    if (!device_path)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to determine DRM device node path from EGLDevice"));
    }
    return device_path;
}
}

mge::Platform::Platform(
    EGLDeviceEXT device,
    std::shared_ptr<EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<DisplayReport> const& /*report*/)
    : device{device},
      display{EGL_NO_DISPLAY},
      drm_node{open(drm_node_for_device(device), O_RDWR | O_CLOEXEC)}
{
    using namespace std::literals;

    if (drm_node == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION(std::system_error(
            errno,
            std::system_category(),
            "Failed to open DRM device "s + drm_node_for_device(device)));
    }

    if (drmSetMaster(drm_node))
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to acquire DRM master"}));
    }

    int const drm_node_attrib[] = {
        EGL_DRM_MASTER_FD_EXT, static_cast<int>(drm_node), EGL_NONE
    };
    display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, drm_node_attrib);

    if (display == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLDisplay on the EGLDeviceEXT"));
    }

    EGLint major{1};
    EGLint minor{4};
    if (eglInitialize(display, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL"));
    }
    if (major != 1 || minor != 4)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Incompatible EGL version"s +
            "Wanted 1.4, got " + std::to_string(major) + "." + std::to_string(minor)}));
    }
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mge::Platform::create_buffer_allocator()
{
    return mir::make_module_ptr<mge::BufferAllocator>();
}

mir::UniqueModulePtr<mg::Display> mge::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return mir::make_module_ptr<mge::Display>(drm_node, display, configuration_policy, *gl_config);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mge::Platform::make_ipc_operations() const
{
    class NoIPCOperations : public mg::PlatformIpcOperations
    {

    public:
        void pack_buffer(BufferIpcMessage& packer, Buffer const& buffer, BufferIpcMsgType msg_type) const override
        {
            if (msg_type == mg::BufferIpcMsgType::full_msg)
            {
                auto native_handle = buffer.native_buffer_handle();
                for(auto i=0; i<native_handle->data_items; i++)
                {
                    packer.pack_data(native_handle->data[i]);
                }
                for(auto i=0; i<native_handle->fd_items; i++)
                {
                    packer.pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
                }

                packer.pack_stride(buffer.stride());
                packer.pack_flags(native_handle->flags);
                packer.pack_size(buffer.size());
            }
        }

        void unpack_buffer(BufferIpcMessage& /*message*/, Buffer const& /*buffer*/) const override
        {

        }

        std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
        {
            return std::make_shared<mg::PlatformIPCPackage>(describe_graphics_module());
        }

        PlatformOperationMessage platform_operation(unsigned int const /*opcode*/,
            PlatformOperationMessage const& /*message*/) override
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"No platform operations implemented"});
        }
    };

    return mir::make_module_ptr<NoIPCOperations>();
}

EGLNativeDisplayType mge::Platform::egl_native_display() const
{
    return display;
}
