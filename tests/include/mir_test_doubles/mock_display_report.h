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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_

#include "mir/graphics/display_report.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplayReport : public graphics::DisplayReport
{
public:
    MOCK_METHOD0(report_successful_setup_of_native_resources, void());
    MOCK_METHOD0(report_successful_egl_make_current_on_construction, void());
    MOCK_METHOD0(report_successful_egl_buffer_swap_on_construction, void());
    MOCK_METHOD0(report_successful_drm_mode_set_crtc_on_construction, void());
    MOCK_METHOD0(report_successful_display_construction, void());
    MOCK_METHOD1(report_drm_master_failure, void(int));
    MOCK_METHOD0(report_vt_switch_away_failure, void());
    MOCK_METHOD0(report_vt_switch_back_failure, void());
    MOCK_METHOD2(report_egl_configuration, void(EGLDisplay,EGLConfig));
    MOCK_METHOD1(report_vsync, void(unsigned int));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_ */
