/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/graphics/animation_driver.h"
#include "mir/time/clock.h"
#include <chrono>

mir::graphics::AnimationDriver::AnimationDriver(
    std::shared_ptr<DisplayReport> const& to_wrap, std::shared_ptr<time::Clock> const& clock) :
    ObserverMultiplexer<AnimationObserver>{mir::linearising_executor},
    wrapped_report{to_wrap},
    clock{clock},
    last_vsync{clock->now()}
{
}

void mir::graphics::AnimationDriver::on_vsync(std::chrono::milliseconds dt)
{
    for_each_observer(&AnimationObserver::on_vsync, dt);
}

void mir::graphics::AnimationDriver::report_successful_setup_of_native_resources()
{
    wrapped_report->report_successful_setup_of_native_resources();
}

void mir::graphics::AnimationDriver::report_successful_egl_make_current_on_construction()
{
    wrapped_report->report_successful_egl_make_current_on_construction();
}

void mir::graphics::AnimationDriver::report_successful_egl_buffer_swap_on_construction()
{
    wrapped_report->report_successful_egl_buffer_swap_on_construction();
}

void mir::graphics::AnimationDriver::report_successful_display_construction()
{
    wrapped_report->report_successful_display_construction();
}

void mir::graphics::AnimationDriver::report_egl_configuration(EGLDisplay disp, EGLConfig cfg)
{
    wrapped_report->report_egl_configuration(disp, cfg);
}

void mir::graphics::AnimationDriver::report_vsync(unsigned int output_id, Frame const& f)
{
    wrapped_report->report_vsync(output_id, f);

    auto const now = clock->now();
    auto const dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_vsync);
    on_vsync(dt);
    last_vsync = now;
}

void mir::graphics::AnimationDriver::report_successful_drm_mode_set_crtc_on_construction()
{
    wrapped_report->report_successful_drm_mode_set_crtc_on_construction();
}

void mir::graphics::AnimationDriver::report_drm_master_failure(int error)
{
    wrapped_report->report_drm_master_failure(error);
}

void mir::graphics::AnimationDriver::report_vt_switch_away_failure()
{
    wrapped_report->report_vt_switch_away_failure();
}

void mir::graphics::AnimationDriver::report_vt_switch_back_failure()
{
    wrapped_report->report_vt_switch_back_failure();
}

