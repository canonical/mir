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
#include "buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/buffer.h"

#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

mga::HWC11Device::HWC11Device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                              std::shared_ptr<HWCVsyncCoordinator> const& coordinator)
    : HWCCommonDevice(hwc_device, coordinator),
      layer_list({mga::CompositionLayer{true}, mga::FramebufferLayer{}}),
      sync_ops(std::make_shared<mga::RealSyncFileOps>())
{
}

void mga::HWC11Device::prepare_composition()
{
    //note, although we only have a primary display right now,
    //      set the second display to nullptr, as exynos hwc always derefs displays[1]
    hwc_display_contents_1_t* displays[HWC_NUM_DISPLAY_TYPES] {layer_list.native_list(), nullptr};

    if (hwc_device->prepare(hwc_device.get(), 1, displays))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc prepare()"));
    }
}

void mga::HWC11Device::gpu_render(EGLDisplay dpy, EGLSurface sur)
{
    if (eglSwapBuffers(dpy, sur) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
    }
}

void mga::HWC11Device::post(mg::Buffer const& buffer)
{
    auto lg = lock_unblanked();

    auto native_buffer = buffer.native_buffer_handle();
    layer_list.set_fb_target(native_buffer);

    hwc_display_contents_1_t* displays[HWC_NUM_DISPLAY_TYPES] {layer_list.native_list(), nullptr};
    if (hwc_device->set(hwc_device.get(), 1, displays))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc set()"));
    }

    if (last_display_fence)
        last_display_fence->wait();

    int framebuffer_fence = layer_list.framebuffer_fence();
    native_buffer->update_fence(framebuffer_fence);

    last_display_fence = std::make_shared<mga::SyncFence>(
        sync_ops, displays[HWC_DISPLAY_PRIMARY]->retireFenceFd);
}
