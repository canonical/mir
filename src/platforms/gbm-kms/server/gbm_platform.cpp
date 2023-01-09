/*
 * Copyright © Canonical Ltd.
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
 */

#include "gbm_platform.h"
#include "buffer_allocator.h"

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;

mgg::GBMPlatform::GBMPlatform(
    std::shared_ptr<mir::udev::Context> const& udev,
    std::shared_ptr<mgg::helpers::DRMHelper> const& drm) :
    udev(udev),
    drm(drm),
    gbm{std::make_shared<mgg::helpers::GBMHelper>(drm->fd)}
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator(
    Display const& output)
{
    return make_module_ptr<mgg::BufferAllocator>(output);
}
