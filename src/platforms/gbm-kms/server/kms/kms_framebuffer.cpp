/*
<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
 * Copyright © Canonical Ltd.
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
 * Copyright © 2017 Canonical Ltd.
========
 * Copyright © 2021 Canonical Ltd.
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp
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

<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
#include "rendering_platform.h"
#include "buffer_allocator.h"
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
#include "gbm_platform.h"
#include "buffer_allocator.h"
========
#include "kms_framebuffer.h"

#include <xf86drmMode.h>
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp

<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
namespace mg = mir::graphics;
namespace mge = mg::egl::generic;
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
========
namespace mgg = mir::graphics::gbm;
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp

<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
mge::RenderingPlatform::RenderingPlatform()
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
mgg::GBMPlatform::GBMPlatform(
    std::shared_ptr<mir::udev::Context> const& udev,
    std::shared_ptr<mgg::helpers::DRMHelper> const& drm) :
    udev(udev),
    drm(drm),
    gbm{std::make_shared<mgg::helpers::GBMHelper>(drm->fd)}
========
mgg::FBHandle::FBHandle(int drm_fd, uint32_t fb_id)
        : drm_fd{drm_fd},
          fb_id{fb_id}
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp
{
}

<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
auto mge::RenderingPlatform::create_buffer_allocator(
    mg::Display const& output) -> mir::UniqueModulePtr<mg::GraphicBufferAllocator>
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator(
    Display const& output)
========
mgg::FBHandle::~FBHandle()
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp
{
<<<<<<<< HEAD:src/platforms/renderer-generic-egl/rendering_platform.cpp
    return make_module_ptr<mge::BufferAllocator>(output);
|||||||| da02aa60d3:src/platforms/gbm-kms/server/gbm_platform.cpp
    return make_module_ptr<mgg::BufferAllocator>(output);
========
    // TODO: Some sort of logging on failure?
>>>>>>>> new-platform-API:src/platforms/gbm-kms/server/kms/kms_framebuffer.cpp
}

auto mgg::FBHandle::get_drm_fb_id() const -> uint32_t
{
    return fb_id;
}

mgg::FBHandle::operator uint32_t() const
{
    return fb_id;
}