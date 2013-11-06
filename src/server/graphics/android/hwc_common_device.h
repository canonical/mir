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

#ifndef MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_

#include "display_device.h"
#include <hardware/hwcomposer.h>

#include <mutex>
#include <condition_variable>

namespace mir
{
namespace graphics
{
namespace android
{

class HWCVsyncCoordinator;
class HWCCommonDevice;
struct HWCCallbacks
{
    hwc_procs_t hooks;
    HWCCommonDevice* self;
};

class HWCCommonDevice : public DisplayDevice
{
public:
    virtual ~HWCCommonDevice() noexcept;

    void notify_vsync();
    void mode(MirPowerMode mode);

protected:
    HWCCommonDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                    std::shared_ptr<HWCVsyncCoordinator> const& coordinator);

    std::shared_ptr<hwc_composer_device_1> const hwc_device;
    std::shared_ptr<HWCVsyncCoordinator> const coordinator;
    std::unique_lock<std::mutex> lock_unblanked();

private:
    int turn_screen_on() const noexcept(true);
    int turn_screen_off() const noexcept(true);

    HWCCallbacks callbacks;

    std::mutex blanked_mutex;
    std::condition_variable blanked_cond;
    MirPowerMode current_mode; 
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_ */
