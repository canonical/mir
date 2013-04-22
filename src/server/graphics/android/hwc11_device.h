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
#include "hwc_device.h"
#include <hardware/hwcomposer.h>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{
class HWC11Device;
class HWCLayerOrganizer;

struct HWCCallbacks
{
    hwc_procs_t hooks;
    HWC11Device* self;
};

class HWC11Device : public HWCDevice
{
public:
    HWC11Device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                std::shared_ptr<HWCLayerOrganizer> const& organizer,
                std::shared_ptr<DisplaySupportProvider> const& fbdev);
    ~HWC11Device() noexcept;

    geometry::Size display_size() const; 
    geometry::PixelFormat display_format() const;
    unsigned int number_of_framebuffers_available() const;
    void set_next_frontbuffer(std::shared_ptr<AndroidBuffer> const& buffer);
 
    void wait_for_vsync();
    void commit_frame();
    void notify_vsync();

private:
    HWCCallbacks callbacks;
    std::shared_ptr<hwc_composer_device_1> const hwc_device;
    std::shared_ptr<HWCLayerOrganizer> const layer_organizer;
    std::shared_ptr<DisplaySupportProvider> const fb_device;
    std::mutex vsync_wait_mutex;
    std::condition_variable vsync_trigger;
    bool vsync_occurred;
    unsigned int primary_display_config;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC11_DEVICE_H_ */
