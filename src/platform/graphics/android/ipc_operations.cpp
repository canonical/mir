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
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/android/android_native_buffer.h"
#include "ipc_operations.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;

void mga::IpcOperations::pack_buffer(BufferIpcMessage& msg, Buffer const& buffer, BufferIpcMsgType msg_type) const
{
    auto native_buffer = buffer.native_buffer_handle();

    mir::Fd fence_fd(native_buffer->copy_fence());
    if (fence_fd != mir::Fd::invalid)
    {
        msg.pack_data(static_cast<int>(mga::BufferFlag::fenced));
        msg.pack_fd(fence_fd);
    }
    else
    {
        msg.pack_data(static_cast<int>(mga::BufferFlag::unfenced));
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

        msg.pack_stride(buffer.stride());
        msg.pack_size(buffer.size());
    }
}

void mga::IpcOperations::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
}

std::shared_ptr<mg::PlatformIPCPackage> mga::IpcOperations::connection_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

mg::PlatformIPCPackage mga::IpcOperations::platform_operation(
    unsigned int const, mg::PlatformIPCPackage const&)
{
    BOOST_THROW_EXCEPTION(std::invalid_argument("android platform does not support any platform operations"));
}
