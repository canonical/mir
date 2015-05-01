/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_REPORT_H_

#include "src/platforms/android/server/hwc_report.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockHwcReport : public graphics::android::HwcReport
{
    MOCK_CONST_METHOD1(report_list_submitted_to_prepare,
        void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays));
    MOCK_CONST_METHOD1(report_prepare_done,
        void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays));
    MOCK_CONST_METHOD1(report_set_list,
        void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays));
    MOCK_CONST_METHOD1(report_set_done,
        void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays)); 
    MOCK_CONST_METHOD1(report_overlay_optimization, void(graphics::android::OverlayOptimization));
    MOCK_CONST_METHOD0(report_display_on, void());
    MOCK_CONST_METHOD0(report_display_off, void());
    MOCK_CONST_METHOD0(report_vsync_on, void());
    MOCK_CONST_METHOD0(report_vsync_off, void());
    MOCK_CONST_METHOD1(report_hwc_version, void(graphics::android::HwcVersion));
    MOCK_CONST_METHOD0(report_legacy_fb_module, void());
    MOCK_CONST_METHOD1(report_power_mode, void(graphics::android::PowerMode));
};
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_REPORT_H_ */
