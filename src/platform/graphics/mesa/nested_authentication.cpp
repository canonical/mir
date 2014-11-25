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
#include "nested_authentication.h"
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mgm = mir::graphics::mesa;

mgm::NestedAuthentication::NestedAuthentication(std::shared_ptr<NestedContext> const& nested_context) :
    nested_context{nested_context}
{
}

void mgm::NestedAuthentication::auth_magic(drm_magic_t magic)
{
    nested_context->drm_auth_magic(magic);
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

    nested_context->drm_auth_magic(magic);
    return mir::Fd(IntOwnedFd{auth_fd});
}
