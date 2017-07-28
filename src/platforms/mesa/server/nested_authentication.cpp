/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform_authentication.h"
#include "mir/graphics/platform_operation_message.h"
#include "nested_authentication.h"

#include "mir_toolkit/mesa/platform_operation.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <mutex>
#include <condition_variable>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

namespace
{
std::shared_ptr<mg::MesaAuthExtension> load_extension(mg::PlatformAuthentication& context)
{
    auto ext = context.auth_extension();
    if (!ext.is_set())
        BOOST_THROW_EXCEPTION(std::runtime_error("No mesa_drm_auth extension. Perhaps the host is not using the mesa platform"));
    return ext.value();
}
}

mgm::NestedAuthentication::NestedAuthentication(
    std::shared_ptr<PlatformAuthentication> const& platform_authentication) :
    platform_authentication{platform_authentication},
    auth_extension(load_extension(*platform_authentication))

{
}

void mgm::NestedAuthentication::auth_magic(drm_magic_t magic)
{
    static int const success{0};
    auto rc = auth_extension->auth_magic(magic);
    if (rc != success)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(rc, std::system_category(), 
                "Nested server failed to authenticate DRM device magic cookie"));
    }
}

mir::Fd mgm::NestedAuthentication::authenticated_fd()
{
    auto fd = auth_extension->auth_fd(); 
    if (fd <= mir::Fd::invalid)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested server failed to get authenticated DRM fd"));
    return fd;
}
