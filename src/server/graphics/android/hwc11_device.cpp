/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc11_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync_coordinator.h"
#include "framebuffer_bundle.h"
#include "mir/graphics/android/sync_fence.h"

#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

mga::HWC11Device::HWC11Device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                              std::shared_ptr<FramebufferBundle> const& fb_bundle,
                              std::shared_ptr<HWCLayerList> const& layer_list,
                              std::shared_ptr<HWCVsyncCoordinator> const& coordinator)
    : HWCCommonDevice(hwc_device, coordinator),
      fb_bundle(fb_bundle),
      layer_list(layer_list),
      sync_ops(std::make_shared<mga::RealSyncFileOps>())
{
    printf("con con 11\n");
}

geom::Size mga::HWC11Device::display_size() const
{
    return fb_bundle->fb_size();
}

geom::PixelFormat mga::HWC11Device::display_format() const
{
    return fb_bundle->fb_format();
}

std::shared_ptr<mg::Buffer> mga::HWC11Device::buffer_for_render()
{
    return fb_bundle->buffer_for_render();
}

void mga::HWC11Device::commit_frame(EGLDisplay dpy, EGLSurface sur)
{
    auto lg = lock_unblanked();

printf("zog.\n");
    //note, although we only have a primary display right now,
    //      set the second display to nullptr, as exynos hwc always derefs displays[1]
    hwc_display_contents_1_t* displays[HWC_NUM_DISPLAY_TYPES] {layer_list->native_list(), nullptr};

    if (hwc_device->prepare(hwc_device.get(), 1, displays))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc prepare()"));
    }

    if (eglSwapBuffers(dpy, sur) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during eglSwapBuffers"));
    }

    /* update gles rendered surface */
    auto buffer = fb_bundle->last_rendered_buffer();
    layer_list->set_fb_target(buffer);

    if (hwc_device->set(hwc_device.get(), 1, displays))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc set()"));
    }

printf("commit and go\n");
    mga::SyncFence fence(sync_ops, displays[HWC_DISPLAY_PRIMARY]->retireFenceFd);
    fence.wait();
}

void mga::HWC11Device::sync_to_display(bool)
{
    //TODO return error code, running not synced to vsync is not supported
}
