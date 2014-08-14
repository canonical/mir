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

#ifndef MIR_GRAPHICS_ANDROID_HWC_LOGGERS_H_
#define MIR_GRAPHICS_ANDROID_HWC_LOGGERS_H_

#include "hwc_logger.h"

namespace mir
{
namespace graphics
{
namespace android
{
class HwcFormattedLogger : public HwcLogger
{
public:
    HwcFormattedLogger() = default;
    void log_list_submitted_to_prepare(hwc_display_contents_1_t const& list) const override;
    void log_prepare_done(hwc_display_contents_1_t const& list) const override;
    void log_set_list(hwc_display_contents_1_t const& list) const override;
    void log_overlay_optimization(OverlayOptimization optimization_option) const override;
    void log_display_on() const override;
    void log_display_off() const override;
    void log_vsync_on() const override;
    void log_vsync_off() const override;
};

class NullHwcLogger : public HwcLogger
{
public:
    NullHwcLogger() = default;
    void log_list_submitted_to_prepare(hwc_display_contents_1_t const&) const override;
    void log_prepare_done(hwc_display_contents_1_t const&) const override;
    void log_set_list(hwc_display_contents_1_t const&) const override;
    void log_overlay_optimization(OverlayOptimization optimization_option) const override;
    void log_display_on() const override;
    void log_display_off() const override;
    void log_vsync_on() const override;
    void log_vsync_off() const override;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_HWC_LOGGERS_H_ */
