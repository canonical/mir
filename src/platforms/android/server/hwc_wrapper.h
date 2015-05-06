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

#ifndef MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_
#define MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_

#include "mir/int_wrapper.h"
#include "display_name.h"
#include "power_mode.h"
#include <array>
#include <functional>
#include <chrono>
#include <vector>

struct hwc_display_contents_1;

namespace mir
{
namespace graphics
{
namespace android
{

struct ConfigIdTag;
typedef IntWrapper<ConfigIdTag, uint32_t> ConfigId;

struct HWCCallbacks;
class HwcWrapper
{
public:
    virtual ~HwcWrapper() = default;

    virtual void prepare(std::array<hwc_display_contents_1*, HWC_NUM_DISPLAY_TYPES> const&) const = 0;
    virtual void set(std::array<hwc_display_contents_1*, HWC_NUM_DISPLAY_TYPES> const&) const = 0;
    //receive vsync, invalidate, and hotplug events from the driver.
    //As with the HWC api, these events MUST NOT call-back to the other functions in HwcWrapper. 
    virtual void subscribe_to_events(
        void const* subscriber,
        std::function<void(DisplayName, std::chrono::nanoseconds)> const& vsync_callback,
        std::function<void(DisplayName, bool)> const& hotplug_callback,
        std::function<void()> const& invalidate_callback) = 0;
    virtual void unsubscribe_from_events(void const* subscriber) noexcept = 0;
    virtual void vsync_signal_on(DisplayName) const = 0;
    virtual void vsync_signal_off(DisplayName) const = 0;
    virtual void display_on(DisplayName) const = 0;
    virtual void display_off(DisplayName) const = 0;
    virtual std::vector<ConfigId> display_configs(DisplayName) const = 0;
    virtual int display_attributes(
        DisplayName, ConfigId, uint32_t const* attributes, int32_t* values) const = 0;
    virtual void power_mode(DisplayName, PowerMode mode) const = 0;
    virtual bool has_active_config(DisplayName) const = 0;
    virtual ConfigId active_config_for(DisplayName name) const = 0;
    virtual void set_active_config(DisplayName name, ConfigId id) const = 0;

protected:
    HwcWrapper() = default;
    HwcWrapper& operator=(HwcWrapper const&) = delete;
    HwcWrapper(HwcWrapper const&) = delete;
};
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_HWC_WRAPPER_H_ */
