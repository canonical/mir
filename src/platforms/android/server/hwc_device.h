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
#include "display_device.h"
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
class SyncFileOps;
class HwcWrapper;
class HwcConfiguration;

class HwcDevice : public DisplayDevice
{
public:
    HwcDevice(std::shared_ptr<HwcWrapper> const& hwc_wrapper);

    bool compatible_renderlist(RenderableList const& renderlist) override;
    void commit(std::list<DisplayContents> const& contents) override;
    void content_cleared() override;
    std::chrono::milliseconds recommended_sleep() const override;

private:
    bool buffer_is_onscreen(Buffer const&) const;
    std::vector<std::shared_ptr<Buffer>> onscreen_overlay_buffers;

    std::shared_ptr<HwcWrapper> const hwc_wrapper;
    std::shared_ptr<SyncFileOps> const sync_ops;
    std::chrono::milliseconds recommend_sleep{0};
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_DEVICE_H_ */
