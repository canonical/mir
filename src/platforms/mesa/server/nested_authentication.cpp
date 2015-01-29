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

#include "mir/graphics/nested_context.h"
#include "mir/graphics/platform_operation_message.h"
#include "nested_authentication.h"

#include "mir_toolkit/mesa/platform_operation.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

namespace
{

mg::PlatformOperationMessage make_auth_magic_request_message(drm_magic_t magic)
{
    mg::PlatformOperationMessage request;
    request.data.resize(sizeof(MirMesaAuthMagicRequest));
    auto auth_magic_request_ptr =
        reinterpret_cast<MirMesaAuthMagicRequest*>(request.data.data());
    auth_magic_request_ptr->magic = magic;
    return request;
}

MirMesaAuthMagicResponse auth_magic_response_from_message(
    mg::PlatformOperationMessage const& msg)
{
    if (msg.data.size() == sizeof(MirMesaAuthMagicResponse))
    {
        auto auth_magic_response_ptr =
            reinterpret_cast<MirMesaAuthMagicResponse const*>(msg.data.data());

        return *auth_magic_response_ptr;
    }

    BOOST_THROW_EXCEPTION(
        std::runtime_error("Nested server got invalid auth magic response"));
}

}

mgm::NestedAuthentication::NestedAuthentication(
    std::shared_ptr<NestedContext> const& nested_context)
    : nested_context{nested_context}
{
}

void mgm::NestedAuthentication::auth_magic(drm_magic_t magic)
{
    static int const success{0};

    auto const request_msg = make_auth_magic_request_message(magic);

    auto const response_msg = nested_context->platform_operation(
        MirMesaPlatformOperation::auth_magic, request_msg);

    auto const auth_magic_response = auth_magic_response_from_message(response_msg);
    if (auth_magic_response.status != success)
    {
        std::string const error_msg{
            "Nested server failed to authenticate DRM device magic cookie"};

        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error(error_msg)) <<
                    boost::errinfo_errno(auth_magic_response.status));
    }
}

mir::Fd mgm::NestedAuthentication::authenticated_fd()
{
    auto fds = nested_context->platform_fd_items();
    if (fds.size() < 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to get fd from platform package"));
    char* busid = drmGetBusid(fds[0]);
    if (!busid)
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get BusID of DRM device")) << boost::errinfo_errno(errno));
    int auth_fd = drmOpen(NULL, busid);
    free(busid);

    drm_magic_t magic;
    int ret = -1;
    if ((ret = drmGetMagic(auth_fd, &magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    auth_magic(magic);
    return mir::Fd(IntOwnedFd{auth_fd});
}
