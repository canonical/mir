/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform_ipc_package.h"
#include "mir/module_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/libname.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "android_native_buffer.h"
#include "ipc_operations.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;

void mga::IpcOperations::pack_buffer(BufferIpcMessage& msg, Buffer const& buffer, BufferIpcMsgType msg_type) const
{
    auto native_buffer = mga::to_native_buffer_checked(buffer.native_buffer_handle());

    native_buffer->wait_for_unlock_by_gpu();
    mir::Fd fence_fd(native_buffer->copy_fence());
    if (fence_fd != mir::Fd::invalid)
    {
        msg.pack_flags(mir_buffer_flag_fenced);
        msg.pack_fd(fence_fd);
    }
    else
    {
        msg.pack_flags(0);
    }

    if (msg_type == mg::BufferIpcMsgType::full_msg)
    {
        auto buffer_handle = native_buffer->handle();
        int offset = 0;

        for(auto i = 0; i < buffer_handle->numFds; i++)
        {
            msg.pack_fd(mir::Fd(IntOwnedFd{buffer_handle->data[offset++]}));
        }
        for(auto i = 0; i < buffer_handle->numInts; i++)
        {
            msg.pack_data(buffer_handle->data[offset++]);
        }

        mir::geometry::Stride byte_stride{
            native_buffer->anwb()->stride * MIR_BYTES_PER_PIXEL(buffer.pixel_format())};
        msg.pack_stride(byte_stride);
        msg.pack_size(buffer.size());
    }
}

void mga::IpcOperations::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
}

namespace
{
mir::ModuleProperties const properties = {
    "mir:android",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

std::shared_ptr<mg::PlatformIPCPackage> mga::IpcOperations::connection_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>(&properties);
}

mg::PlatformOperationMessage mga::IpcOperations::platform_operation(
    unsigned int const, mg::PlatformOperationMessage const&)
{
    BOOST_THROW_EXCEPTION(std::invalid_argument("android platform does not support any platform operations"));
}
