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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_
#include "hwc_device.h"
#include <hardware/hwcomposer.h>

namespace mir
{
namespace graphics
{
namespace android
{
class HWCLayerOrganizer;
class FBDevice;

class HWCCommonDevice : public HWCDevice
{
public:
    HWCCommonDevice();
    ~HWCCommonDevice() noexcept;

    geometry::PixelFormat display_format() const;
    unsigned int number_of_framebuffers_available() const;
    void set_next_frontbuffer(std::shared_ptr<AndroidBuffer> const& buffer);
 
    void wait_for_vsync();
    void notify_vsync();
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_COMMON_DEVICE_H_ */
