/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_HWC_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC_DEVICE_H_

#include "mir_toolkit/common.h"
#include "mir/graphics/android/sync_fence.h"
#include "hwc_common_device.h"
#include "hwc_layerlist.h"
#include <memory>
#include <vector>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{
class HWCVsyncCoordinator;
class SyncFileOps;
class HwcWrapper;
class HwcConfiguration;

class HwcDevice : public HWCCommonDevice
{
public:
    HwcDevice(
        std::shared_ptr<HwcWrapper> const& hwc_wrapper,
        std::shared_ptr<HwcConfiguration> const& hwc_config,
        std::shared_ptr<HWCVsyncCoordinator> const& coordinator,
        std::shared_ptr<LayerAdapter> const& layer_adapter);

    virtual void post_gl(SwappingGLContext const& context) override;
    virtual bool post_overlays(
        SwappingGLContext const& context,
        RenderableList const& list,
        RenderableListCompositor const& list_compositor) override;

private:
    bool buffer_is_onscreen(Buffer const&) const;
    void turned_screen_off() override;
    LayerList hwc_list;
    std::vector<std::shared_ptr<Buffer>> onscreen_overlay_buffers;

    std::shared_ptr<HwcWrapper> const hwc_wrapper;
    std::shared_ptr<SyncFileOps> const sync_ops;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_DEVICE_H_ */
