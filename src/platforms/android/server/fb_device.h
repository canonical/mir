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

#ifndef MIR_GRAPHICS_ANDROID_FB_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_FB_DEVICE_H_

#include "display_device.h"
#include "hwc_configuration.h"
#include <hardware/gralloc.h>
#include <hardware/fb.h>

namespace mir
{
namespace graphics
{
namespace android
{

class FbControl : public HwcConfiguration
{
public:
    FbControl(std::shared_ptr<framebuffer_device_t> const& fbdev);
    void power_mode(DisplayName, MirPowerMode) override;
    DisplayConfigurationOutput active_config_for(DisplayName) override;
    ConfigChangeSubscription subscribe_to_config_changes(
        std::function<void()> const& hotplug_cb,
        std::function<void(DisplayName)> const& vsync_cb) override;
private:
    std::shared_ptr<framebuffer_device_t> const fb_device;
};

class FBDevice : public DisplayDevice
{
public:
    FBDevice(std::shared_ptr<framebuffer_device_t> const& fbdev);

    bool compatible_renderlist(RenderableList const& renderlist) override;
    void commit(std::list<DisplayContents> const& contents) override;
    std::chrono::milliseconds recommended_sleep() const override;

private:
    std::shared_ptr<framebuffer_device_t> const fb_device;
    void content_cleared() override;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_FB_DEVICE_H_ */
