/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_

#include "mir/graphics/display_report.h"
#include "mir/graphics/frame.h"  // GMock can't live with just forward decls
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
    MOCK_METHOD(void, report_successful_setup_of_native_resources, (), (override));
    MOCK_METHOD(void, report_successful_egl_make_current_on_construction, (), (override));
    MOCK_METHOD(void, report_successful_egl_buffer_swap_on_construction, (), (override));
    MOCK_METHOD(void, report_successful_drm_mode_set_crtc_on_construction, (), (override));
    MOCK_METHOD(void, report_successful_display_construction, (), (override));
    MOCK_METHOD(void, report_drm_master_failure, (int), (override));
    MOCK_METHOD(void, report_vt_switch_away_failure, (), (override));
    MOCK_METHOD(void, report_vt_switch_back_failure, (), (override));
    MOCK_METHOD(void, report_egl_configuration, (EGLDisplay,EGLConfig), (override));
    MOCK_METHOD(void, report_vsync, (unsigned int, graphics::Frame const&), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_REPORT_H_ */
