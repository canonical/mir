/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_DISPLAY_REPORT_H_
#define MIR_GRAPHICS_DISPLAY_REPORT_H_

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{

class DisplayReport
{
public:

    virtual void report_successful_setup_of_native_resources() = 0;
    virtual void report_successful_egl_make_current_on_construction() = 0;
    virtual void report_successful_egl_buffer_swap_on_construction() = 0;
    virtual void report_successful_display_construction() = 0;
    virtual void report_egl_configuration(EGLDisplay disp, EGLConfig cfg) = 0;
    virtual void report_vsync(unsigned int display_id) = 0;

    /* gbm specific */
    virtual void report_successful_drm_mode_set_crtc_on_construction() = 0;
    virtual void report_drm_master_failure(int error) = 0;
    virtual void report_vt_switch_away_failure() = 0;
    virtual void report_vt_switch_back_failure() = 0;

protected:
    DisplayReport() = default;
    virtual ~DisplayReport() = default;
    DisplayReport(const DisplayReport&) = delete;
    DisplayReport& operator=(const DisplayReport&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_REPORT_H_ */
