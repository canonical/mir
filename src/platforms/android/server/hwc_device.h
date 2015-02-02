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
#include <mutex>
#include <condition_variable>

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
    void commit(
        DisplayName,
        LayerList&,
        SwappingGLContext const& context,
        RenderableListCompositor const& list_compositor) override;
    void content_cleared() override;
    void start_posting_external_display() override;
    void stop_posting_external_display() override;

private:
    void commit();
    bool buffer_is_onscreen(Buffer const&) const;
    std::vector<std::shared_ptr<Buffer>> onscreen_overlay_buffers;

    std::shared_ptr<HwcWrapper> const hwc_wrapper;
    std::shared_ptr<SyncFileOps> const sync_ops;

    std::mutex mutex;

    std::condition_variable commit_cv;
    bool committed;

    struct ListResources
    {
        DisplayName name;
        LayerList& list;
        SwappingGLContext const& context;
        RenderableListCompositor const& compositor;
    };
    std::condition_variable list_cv;
    std::list<ListResources> displays;
    size_t needed_list_count;

    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> lists; 
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_DEVICE_H_ */
