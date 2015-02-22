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

#ifndef MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_

#include "display_device.h"
#include <hardware/hwcomposer.h>

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace mir
{
namespace graphics
{
namespace android
{

class HwcWrapper;
class HWCVsyncCoordinator;
class HWCCommonDevice;
struct HWCCallbacks
{
    hwc_procs_t hooks;
    std::atomic<HWCCommonDevice*> self;
};

class HWCCommonDevice : public DisplayDevice
{
public:
    virtual ~HWCCommonDevice() noexcept;

    void notify_vsync();
    void mode(MirPowerMode mode);
    bool apply_orientation(MirOrientation orientation) const;

protected:
    HWCCommonDevice(std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                    std::shared_ptr<HWCVsyncCoordinator> const& coordinator);

    std::shared_ptr<HWCVsyncCoordinator> const coordinator;
    std::unique_lock<std::mutex> lock_unblanked();

private:
    void turn_screen_on() const;
    void turn_screen_off();
    virtual void turned_screen_off();

    std::shared_ptr<HWCCallbacks> const callbacks;
    std::shared_ptr<HwcWrapper> const hwc_device;

    std::mutex blanked_mutex;
    std::condition_variable blanked_cond;
    MirPowerMode current_mode;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_ */
