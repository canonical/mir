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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_HWC11_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC11_DEVICE_H_
#include "hwc_common_device.h"
#include "hwc_layerlist.h"
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{
class HWCVsyncCoordinator;
class SyncFileOps;
class FramebufferBundle;

class HWC11Device : public HWCCommonDevice
{
public:
    //TODO, (kdub) might make more sense for fb bundle to be owned and managed by LayerList, once
    //      we start doing overlays
    HWC11Device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                std::shared_ptr<FramebufferBundle> const& fb_bundle,
                std::shared_ptr<HWCVsyncCoordinator> const& coordinator);

    geometry::Size display_size() const; 
    geometry::PixelFormat display_format() const; 

    std::shared_ptr<graphics::Buffer> buffer_for_render();
    void sync_to_display(bool sync); 
    void commit_frame(EGLDisplay dpy, EGLSurface sur);

private:
    std::shared_ptr<FramebufferBundle> const fb_bundle;
    LayerList layer_list;

    std::shared_ptr<SyncFileOps> const sync_ops;
    unsigned int primary_display_config;
    geometry::PixelFormat fb_format;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC11_DEVICE_H_ */
