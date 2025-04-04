/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_ANIMATION_DRIVER_H_
#define MIR_GRAPHICS_ANIMATION_DRIVER_H_

#include "mir/graphics/display_report.h"
#include "mir/observer_multiplexer.h"
#include "mir/time/types.h"

#include <chrono>
#include <memory>

namespace mir
{
namespace time
{
class Clock;
}
namespace graphics
{
class AnimationObserver
{
public:
    virtual ~AnimationObserver() = default;
    virtual void on_vsync(std::chrono::milliseconds dt) = 0;
};

class AnimationDriver: public DisplayReport, public ObserverMultiplexer<AnimationObserver>
{
public:
    AnimationDriver(std::shared_ptr<DisplayReport> const& to_wrap,
            std::shared_ptr<time::Clock> const& clock);

    void on_vsync(std::chrono::milliseconds dt) override;

    void report_successful_setup_of_native_resources() override;
    void report_successful_egl_make_current_on_construction() override;
    void report_successful_egl_buffer_swap_on_construction() override;
    void report_successful_display_construction() override;
    void report_egl_configuration(EGLDisplay disp, EGLConfig cfg) override;
    void report_vsync(unsigned int output_id, Frame const& f) override;
    void report_successful_drm_mode_set_crtc_on_construction() override;
    void report_drm_master_failure(int error) override;
    void report_vt_switch_away_failure() override;
    void report_vt_switch_back_failure() override;

private:
    std::shared_ptr<DisplayReport> const wrapped_report;
    std::shared_ptr<time::Clock> const clock;

    time::Timestamp last_vsync;
    std::shared_ptr<AnimationObserver> const test_observer;
};
}
}

#endif
