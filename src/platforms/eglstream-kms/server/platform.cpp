/*
 * Copyright © 2016 Canonical Ltd.
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
#include "native_buffer.h"

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

mge::DisplayPlatform::DisplayPlatform(EGLDeviceEXT device)
    : display{EGL_NO_DISPLAY},
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
    auto const required_egl_version_major = major;
    auto const required_egl_version_minor = minor;
    if (eglInitialize(display, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL"));
    }
    if ((major < required_egl_version_major) ||
        (major == required_egl_version_major && minor < required_egl_version_minor))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "Incompatible EGL version"s +
            "Wanted 1.4, got " + std::to_string(major) + "." + std::to_string(minor)}));
    }
}

mir::UniqueModulePtr<mg::Display> mge::DisplayPlatform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return mir::make_module_ptr<mge::Display>(drm_node, display, configuration_policy, *gl_config);
}

mg::NativeDisplayPlatform* mge::DisplayPlatform::native_display_platform()
{
    return nullptr;
}

std::vector<mir::ExtensionDescription> mge::DisplayPlatform::extensions() const
{
    return
    {
        { "mir_extension_graphics_module", { 1 } }
    };
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mge::RenderingPlatform::create_buffer_allocator()
{
    return mir::make_module_ptr<mge::BufferAllocator>();
}

mg::NativeRenderingPlatform* mge::RenderingPlatform::native_rendering_platform()
{
    return nullptr;
}
mir::UniqueModulePtr<mg::PlatformIpcOperations> mge::RenderingPlatform::make_ipc_operations() const
{
    class NoIPCOperations : public mg::PlatformIpcOperations
    {

    public:
        void pack_buffer(BufferIpcMessage& packer, Buffer const& buffer, BufferIpcMsgType msg_type) const override
        {
            if (msg_type == mg::BufferIpcMsgType::full_msg)
            {
                auto native_handle = std::dynamic_pointer_cast<mge::NativeBuffer>(buffer.native_buffer_handle());
                if (!native_handle)
                    BOOST_THROW_EXCEPTION(std::invalid_argument{"could not convert NativeBuffer"});
                for(auto i=0; i<native_handle->data_items; i++)
                {
                    packer.pack_data(native_handle->data[i]);
                }
                for(auto i=0; i<native_handle->fd_items; i++)
                {
                    packer.pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
                }

                packer.pack_stride(mir::geometry::Stride{native_handle->stride});
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

mge::Platform::Platform(
    std::shared_ptr<RenderingPlatform> const& rendering,
    std::shared_ptr<DisplayPlatform> const& display) :
    rendering(rendering),
    display(display)
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mge::Platform::create_buffer_allocator()
{
    return rendering->create_buffer_allocator();
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mge::Platform::make_ipc_operations() const
{
    return rendering->make_ipc_operations();
}

mg::NativeRenderingPlatform* mge::Platform::native_rendering_platform()
{
    return rendering->native_rendering_platform();
}

mir::UniqueModulePtr<mg::Display> mge::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& policy,
    std::shared_ptr<GLConfig> const& config)
{
    return display->create_display(policy, config);
}

mg::NativeDisplayPlatform* mge::Platform::native_display_platform()
{
    return display->native_display_platform();
}

std::vector<mir::ExtensionDescription> mge::Platform::extensions() const
{
    return display->extensions();
}
