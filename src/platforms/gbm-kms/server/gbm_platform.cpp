/*
 * Copyright © 2017 Canonical Ltd.
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

#include "gbm_platform.h"
#include "mir/graphics/platform_authentication.h"
#include "buffer_allocator.h"
#include "nested_authentication.h"
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;

namespace
{
mir::Fd drm_fd_from_authentication(mg::PlatformAuthentication& authenticator)
{
    auto master = authenticator.drm_fd();
    auto auth = authenticator.auth_extension();
    if (master.is_set())
    {
        return master.value();
    }
    else if (auth.is_set())
    {
        return auth.value()->auth_fd();
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::logic_error("no authentication fd to make gbm-kms buffers"));
    }
}
}

mgg::GBMPlatform::GBMPlatform(
    std::shared_ptr<mg::PlatformAuthentication> const& platform_authentication) :
    platform_authentication(platform_authentication),
    gbm{std::make_shared<mgg::helpers::GBMHelper>(drm_fd_from_authentication(*platform_authentication))},
    auth{std::make_shared<mgg::NestedAuthentication>(platform_authentication)}
{
    auto gbm_extension = platform_authentication->set_gbm_extension();
    if (gbm_extension.is_set())
        gbm_extension.value()->set_gbm_device(gbm->device);
}

mgg::GBMPlatform::GBMPlatform(
    std::shared_ptr<mir::udev::Context> const& udev,
    std::shared_ptr<mgg::helpers::DRMHelper> const& drm) :
    udev(udev),
    drm(drm),
    gbm{std::make_shared<mgg::helpers::GBMHelper>(drm->fd)},
    auth{drm}
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator(
    Display const& output)
{
    return make_module_ptr<mgg::BufferAllocator>(output);
}

MirServerEGLNativeDisplayType mgg::GBMPlatform::egl_native_display() const
{
    return gbm->device;
}
