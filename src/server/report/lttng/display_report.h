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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_REPORT_LTTNG_DISPLAY_REPORT_H_
#define MIR_REPORT_LTTNG_DISPLAY_REPORT_H_

#include "server_tracepoint_provider.h"

#include "mir/graphics/display_report.h"

namespace mir
{
namespace report
{
namespace lttng
{

class DisplayReport : public graphics::DisplayReport
{
public:
    DisplayReport() = default;
    virtual ~DisplayReport() noexcept(true) = default;

    virtual void report_successful_setup_of_native_resources() override;
    virtual void report_successful_egl_make_current_on_construction() override;
    virtual void report_successful_egl_buffer_swap_on_construction() override;
    virtual void report_successful_display_construction() override;
    virtual void report_egl_configuration(EGLDisplay disp, EGLConfig cfg) override;
    virtual void report_successful_drm_mode_set_crtc_on_construction() override;
    virtual void report_drm_master_failure(int error) override;
    virtual void report_vt_switch_away_failure() override;
    virtual void report_vt_switch_back_failure() override;
    virtual void report_vsync(unsigned int display_id) override;

private:
    ServerTracepointProvider tp_provider;
};

}
}
}

#endif

