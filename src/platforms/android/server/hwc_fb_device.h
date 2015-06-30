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

#ifndef MIR_GRAPHICS_ANDROID_HWC_FB_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC_FB_DEVICE_H_

#include "display_device.h"
#include "hardware/gralloc.h"
#include "hardware/fb.h"
#include "mir/raii.h"
#include "display_name.h"
#include <mutex>
#include <condition_variable>

namespace mir
{
namespace graphics
{
namespace android
{
class HwcWrapper;

class HwcFbDevice : public DisplayDevice 
{
public:
    HwcFbDevice(std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                std::shared_ptr<framebuffer_device_t> const& fb_device);

    bool compatible_renderlist(RenderableList const& renderlist) override;
    void commit(std::list<DisplayContents> const& contents) override;
    std::chrono::milliseconds recommended_sleep() const override;

private:
    void content_cleared() override;
    std::shared_ptr<HwcWrapper> const hwc_wrapper;
    std::shared_ptr<framebuffer_device_t> const fb_device;
    static int const num_displays{1};

    mir::raii::PairedCalls<std::function<void()>, std::function<void()>> vsync_subscription;
    std::mutex vsync_wait_mutex;
    std::condition_variable vsync_trigger;
    bool vsync_occurred;
    void notify_vsync(DisplayName, std::chrono::nanoseconds);
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_FB_DEVICE_H_ */
