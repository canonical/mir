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

namespace mgg=mir::graphics::gbm;
namespace mg=mir::graphics;
namespace mc=mir::compositor;

mgg::GBMPlatform::GBMPlatform()
{
    drm.setup();
    gbm.setup(drm);
}

std::shared_ptr<mg::Platform> mg::create_platform()
{
    return std::make_shared<mgg::GBMPlatform>();
}

std::shared_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator(
        const std::shared_ptr<mg::Platform>& platform)
{
    return std::make_shared<mgg::GBMBufferAllocator>(
            std::static_pointer_cast<mgg::GBMPlatform>(platform));
}

std::shared_ptr<mg::Display> mg::create_display(
        const std::shared_ptr<Platform>& platform)
{
    return std::make_shared<mgg::GBMDisplay>(
            std::static_pointer_cast<mgg::GBMPlatform>(platform));
}
