/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "platform.h"
#include "host_connection.h"
#include "display.h"
#include "platform_image_factory.h"
#include "buffer.h"
#include "native_buffer.h"
#include "mir/shared_library.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"
#include <system/window.h>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;

mgn::Platform::Platform(
    std::shared_ptr<mir::SharedLibrary> const& library, 
    std::shared_ptr<mgn::HostConnection> const& connection, 
    std::shared_ptr<mg::DisplayReport> const& display_report):
    library(library),
    connection(connection),
    display_report(display_report),
    guest_platform(library->load_function<mg::CreateGuestPlatform>(
        "create_guest_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION)(display_report, connection))
{
}

namespace mir {
namespace graphics {
namespace nested {
struct GraphicBufferAllocator : graphics::GraphicBufferAllocator
{
    GraphicBufferAllocator(std::shared_ptr<HostConnection> const& connection) :
        connection(connection)
    {
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(
        BufferProperties const& buffer_properties)
    {
        printf("ALLAC A BUFF\n");
        auto factory = std::make_shared<mgn::AndroidImageFactory>();
        return std::make_shared<mgn::Buffer>(connection, factory, buffer_properties);
    }

    virtual std::vector<MirPixelFormat> supported_pixel_formats()
    {
        return {mir_pixel_format_abgr_8888};
    }

    std::shared_ptr<HostConnection> const connection;
};

mir::ModuleProperties const properties = {
    "mir:android:nested",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    "stus stews"
};
struct PlatformIpcOperations : graphics::PlatformIpcOperations
{
    void pack_buffer(mg::BufferIpcMessage& msg, mg::Buffer const& buffer, mg::BufferIpcMsgType msg_type) const
    {
        printf("PACKAGING\n");
        msg.pack_data(0); //mga unfenced
        if (msg_type == mg::BufferIpcMsgType::full_msg)
        {
            printf("BID %i\n", buffer.id().as_value());
            auto b = reinterpret_cast<mgn::NativeBuffer*>(buffer.native_buffer_handle().get());
            auto bb = b->get_native_handle();


            ANativeWindowBuffer* bh = (ANativeWindowBuffer*)bb;
            auto buffer_handle = bh->handle;
            int offset = 0;

            for(auto i = 0; i < buffer_handle->numFds; i++)
            {
                msg.pack_fd(mir::Fd(IntOwnedFd{buffer_handle->data[offset++]}));
            }
            for(auto i = 0; i < buffer_handle->numInts; i++)
            {
                msg.pack_data(buffer_handle->data[offset++]);
            }

            msg.pack_stride(mir::geometry::Stride{bh->stride * 4});
            msg.pack_size(buffer.size());
        }
    }
    void unpack_buffer(mg::BufferIpcMessage& message, mg::Buffer const& buffer) const
    {
        (void)message; (void)buffer;
    }

    std::shared_ptr<PlatformIPCPackage> connection_ipc_package()
    {
        printf("BADA\n");
        return std::make_shared<mg::PlatformIPCPackage>(&properties);
    }

    PlatformOperationMessage platform_operation(
        unsigned int const opcode, PlatformOperationMessage const& message)
    {
        return {{},{}};
        (void)opcode; (void)message;
    }
};





}}}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgn::Platform::create_buffer_allocator()
{
    return mir::make_module_ptr<GraphicBufferAllocator>(connection);
//    return guest_platform->create_buffer_allocator();
}

mir::UniqueModulePtr<mg::Display> mgn::Platform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& policy,
    std::shared_ptr<mg::GLConfig> const& config)
{
    return mir::make_module_ptr<mgn::Display>(
        guest_platform,
        connection,
        display_report,
        policy,
        config);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgn::Platform::make_ipc_operations() const
{
    return mir::make_module_ptr<mgn::PlatformIpcOperations>();
    //return guest_platform->make_ipc_operations();
}
