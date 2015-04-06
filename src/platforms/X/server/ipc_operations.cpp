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
 *
 */

#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "ipc_operations.h"
#include "../debug.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgx = mg::X;

mgx::IpcOperations::IpcOperations()
{
    CALLED
}

void mgx::IpcOperations::pack_buffer(
    mg::BufferIpcMessage& /*packer*/, Buffer const& /*buffer*/, BufferIpcMsgType /*msg_type*/) const
{
    CALLED
#if 0
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
#endif
}

void mgx::IpcOperations::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
    CALLED
}

mg::PlatformOperationMessage mgx::IpcOperations::platform_operation(
    unsigned int const /*op*/, mg::PlatformOperationMessage const& /*request*/)
{
    CALLED
#if 0
    if (op == MirMesaPlatformOperation::auth_magic)
    {
        MirMesaAuthMagicRequest auth_magic_request;
        if (request.data.size() == sizeof(auth_magic_request))
        {
            std::memcpy(&auth_magic_request, request.data.data(), request.data.size());
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Invalid request message for auth_magic platform operation"));
        }

        MirMesaAuthMagicResponse auth_magic_response{-1};

        try
        {
            drm_auth->auth_magic(auth_magic_request.magic);
            auth_magic_response.status = 0;
        }
        catch (std::exception const& e)
        {
            auto errno_ptr = boost::get_error_info<boost::errinfo_errno>(e);

            if (errno_ptr != nullptr)
                auth_magic_response.status = *errno_ptr;
            else
                throw;
        }

        mg::PlatformOperationMessage response_msg;
        response_msg.data.resize(sizeof(auth_magic_response));
        std::memcpy(response_msg.data.data(), &auth_magic_response, sizeof(auth_magic_response));
        return response_msg;
    }
    else if (op == MirMesaPlatformOperation::auth_fd)
    {
        if (request.data.size() != 0 || request.fds.size() != 0)
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Invalid request message for auth_fd platform operation"));
        }

        return mg::PlatformOperationMessage{{},{drm_auth->authenticated_fd()}};
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Invalid platform operation"));
    }
#else
    BOOST_THROW_EXCEPTION(std::invalid_argument("X platform does not support any platform operations"));
#endif
}

std::shared_ptr<mg::PlatformIPCPackage> mgx::IpcOperations::connection_ipc_package()
{
    CALLED
//    return std::make_shared<MesaPlatformIPCPackage>(drm_auth->authenticated_fd());
    return nullptr;
}
