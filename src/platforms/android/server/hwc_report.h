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

#ifndef MIR_GRAPHICS_ANDROID_HWC_REPORT_H_
#define MIR_GRAPHICS_ANDROID_HWC_REPORT_H_

#include "overlay_optimization.h"
#include "display_resource_factory.h"
#include "power_mode.h"
#include <hardware/hwcomposer.h>

namespace mir
{
namespace graphics
{
namespace android
{
class HwcReport
{
public:
    virtual ~HwcReport() = default;

    virtual void report_list_submitted_to_prepare(
        std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const = 0;
    virtual void report_prepare_done(
        std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const = 0;
    virtual void report_set_list(
        std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const = 0;
    virtual void report_set_done(
        std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const = 0;
    virtual void report_overlay_optimization(OverlayOptimization optimization_option) const = 0;
    virtual void report_display_on() const = 0;
    virtual void report_display_off() const = 0;
    virtual void report_vsync_on() const = 0;
    virtual void report_vsync_off() const = 0;
    virtual void report_hwc_version(HwcVersion) const = 0;
    virtual void report_legacy_fb_module() const = 0;
    virtual void report_power_mode(PowerMode mode) const = 0;

    void set_version(HwcVersion version) { hwc_version = version; }

protected:
    HwcReport() = default;
    HwcReport& operator=(HwcReport const&) = delete;
    HwcReport(HwcReport const&) = delete;
    HwcVersion hwc_version{unknown};
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_HWC_REPORT_H_ */
