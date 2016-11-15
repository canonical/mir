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
#include "mir_toolkit/extensions/mesa_drm.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <mutex>
#include <condition_variable>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

namespace
{

#if 0
mg::PlatformOperationMessage make_auth_magic_request_message(drm_magic_t magic)
{
    MirMesaAuthMagicRequest const request{magic};
    mg::PlatformOperationMessage request_msg;
    request_msg.data.resize(sizeof(request));
    std::memcpy(request_msg.data.data(), &request, sizeof(request));
    return request_msg;
}

MirMesaAuthMagicResponse auth_magic_response_from_message(
    mg::PlatformOperationMessage const& msg)
{
    if (msg.data.size() == sizeof(MirMesaAuthMagicResponse))
    {
        MirMesaAuthMagicResponse response{-1};
        std::memcpy(&response, msg.data.data(), msg.data.size());

        return response;
    }

    BOOST_THROW_EXCEPTION(
        std::runtime_error("Nested server got invalid auth magic response"));
}
#endif
}

mgm::NestedAuthentication::NestedAuthentication(
    std::shared_ptr<NestedContext> const& nested_context)
    : nested_context{nested_context}
{
}

void mgm::NestedAuthentication::auth_magic(drm_magic_t magic)
{
    static int const success{0};

    int rc = -1;
    auto ext = nested_context->auth_extensions();
        printf("AUTH THE MAGIC\n");
    if (ext.is_set())
    {
        rc = ext.value()->auth_magic(magic);
    }

    if (rc != success)
    {
        std::string const error_msg{
            "Nested server failed to authenticate DRM device magic cookie"};
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error(error_msg)) <<
                    boost::errinfo_errno(rc));
    }
#if 0
    auto const request_msg = make_auth_magic_request_message(magic);

    auto const response_msg = nested_context->platform_operation(
        MirMesaPlatformOperation::auth_magic, request_msg);

    auto const auth_magic_response = auth_magic_response_from_message(response_msg);
    if (auth_magic_response.status != success)
    {

    }
#endif
}

mir::Fd mgm::NestedAuthentication::authenticated_fd()
{
    printf("AUTH THE FD\n");
#if 0
    mg::PlatformOperationMessage request_msg;
    auto const response_msg = nested_context->platform_operation(
        MirMesaPlatformOperation::auth_fd, request_msg);

    int auth_fd{-1};
    printf("PASSIn\n"); 
    if (response_msg.fds.size() == 1)
    {
        auth_fd = response_msg.fds[0];
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Nested server failed to get authenticated DRM fd"));
    }
#else

    auto ext = nested_context->auth_extensions();
    if (ext.is_set())
    {
        return ext.value()->auth_fd();
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Nested server failed to get authenticated DRM fd"));
    }
//    virtual mir::optional_value<std::unique_ptr<MesaAuthExtensions>> auth_extensions() = 0;
#endif

}
