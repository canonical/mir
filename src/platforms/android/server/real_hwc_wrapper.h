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

#ifndef MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_
#define MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_

#include "hwc_wrapper.h"
#include <memory>
#include <hardware/hwcomposer.h>

#include <mutex>
#include <unordered_map>

namespace mir
{
namespace graphics
{
namespace android
{
class HwcReport;
class RealHwcWrapper;
struct HwcCallbacks
{
    hwc_procs_t hooks;
    RealHwcWrapper* self;
};

class RealHwcWrapper : public HwcWrapper
{
public:
    RealHwcWrapper(
        //should probably be unique_ptr
        std::shared_ptr<hwc_composer_device_1> const& hwc_device,
        std::shared_ptr<HwcReport> const& report);
    ~RealHwcWrapper();

    void subscribe_to_events(
        void const* subscriber,
        std::function<void(DisplayName, std::chrono::nanoseconds)> const& vsync_callback,
        std::function<void(DisplayName, bool)> const& hotplug_callback,
        std::function<void()> const& invalidate_callback) override;
    void unsubscribe_from_events(void const* subscriber) noexcept override;

    void prepare(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const override;
    void set(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const override;
    void vsync_signal_on(DisplayName) const override;
    void vsync_signal_off(DisplayName) const override;
    void display_on(DisplayName) const override;
    void display_off(DisplayName) const override;
    std::vector<ConfigId> display_configs(DisplayName) const override;
    int display_attributes(
        DisplayName, ConfigId, uint32_t const* attributes, int32_t* values) const override;
    void power_mode(DisplayName , PowerMode mode) const override;
    bool has_active_config(DisplayName name) const override;
    ConfigId active_config_for(DisplayName name) const override;
    void set_active_config(DisplayName name, ConfigId id) const override;

    void vsync(DisplayName, std::chrono::nanoseconds) noexcept;
    void hotplug(DisplayName, bool) noexcept;
    void invalidate() noexcept;

    bool display_connected(DisplayName) const;
private:
    std::shared_ptr<hwc_composer_device_1> const hwc_device;
    std::shared_ptr<HwcReport> const report;
    std::mutex callback_map_lock;
    struct Callbacks
    {
        std::function<void(DisplayName, std::chrono::nanoseconds)> vsync;
        std::function<void(DisplayName, bool)> hotplug;
        std::function<void()> invalidate;
    };
    std::unordered_map<void const*, Callbacks> callback_map;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_ */
