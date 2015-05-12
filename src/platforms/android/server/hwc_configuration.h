/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_
#define MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include "mir/geometry/size.h"
#include "display_name.h"
#include <memory>
#include <functional>

namespace mir
{
namespace graphics
{
namespace android
{

using ConfigChangeSubscription = std::shared_ptr<void>;
//interface adapting for the blanking interface differences between fb, HWC 1.0-1.3, and HWC 1.4+
class HwcConfiguration
{
public:
    virtual ~HwcConfiguration() = default;
    virtual void power_mode(DisplayName, MirPowerMode) = 0;
    virtual DisplayConfigurationOutput active_config_for(DisplayName) = 0; 
    virtual ConfigChangeSubscription subscribe_to_config_changes(
        std::function<void()> const& hotplug_cb,
        std::function<void(DisplayName)> const& vsync_cb) = 0;

protected:
    HwcConfiguration() = default;
    HwcConfiguration(HwcConfiguration const&) = delete;
    HwcConfiguration& operator=(HwcConfiguration const&) = delete;
};

class HwcWrapper;
class HwcBlankingControl : public HwcConfiguration
{
public:
    HwcBlankingControl(std::shared_ptr<HwcWrapper> const&);
    void power_mode(DisplayName, MirPowerMode) override;
    DisplayConfigurationOutput active_config_for(DisplayName) override;
    ConfigChangeSubscription subscribe_to_config_changes(
        std::function<void()> const& hotplug_cb,
        std::function<void(DisplayName)> const& vsync_cb) override;

private:
    std::shared_ptr<HwcWrapper> const hwc_device;
    bool off;
    MirPixelFormat format;
};

class HwcWrapper;
class HwcPowerModeControl : public HwcConfiguration
{
public:
    HwcPowerModeControl(std::shared_ptr<HwcWrapper> const&);
    void power_mode(DisplayName, MirPowerMode) override;
    DisplayConfigurationOutput active_config_for(DisplayName) override;
    ConfigChangeSubscription subscribe_to_config_changes(
        std::function<void()> const& hotplug_cb,
        std::function<void(DisplayName)> const& vsync_cb) override;

private:
    std::shared_ptr<HwcWrapper> const hwc_device;
    MirPixelFormat format;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_ */
