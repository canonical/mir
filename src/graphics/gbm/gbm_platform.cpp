/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/gbm/gbm_platform.h"
#include "mir/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/gbm/gbm_display.h"
#include "mir/graphics/platform_ipc_package.h"

namespace mgg=mir::graphics::gbm;
namespace mg=mir::graphics;
namespace mc=mir::compositor;

mgg::GBMPlatform::GBMPlatform()
{
    drm.setup();
    gbm.setup(drm);
}

std::shared_ptr<mc::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator()
{
    return std::make_shared<mgg::GBMBufferAllocator>(this->shared_from_this());
}

std::shared_ptr<mg::Display> mgg::GBMPlatform::create_display()
{
    return std::make_shared<mgg::GBMDisplay>(this->shared_from_this());
}

std::shared_ptr<mg::PlatformIPCPackage> mgg::GBMPlatform::get_ipc_package()
{
    auto pkg = std::make_shared<mg::PlatformIPCPackage>();

    pkg->ipc_fds.push_back(drm.get_authenticated_fd());

    return pkg;
}

std::shared_ptr<mg::Platform> mg::create_platform()
{
    return std::make_shared<mgg::GBMPlatform>();
}
