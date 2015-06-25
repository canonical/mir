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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/report/logging/display_report.h"
#include "mir/logging/logger.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/advanceable_clock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

namespace ml  = mir::logging;
namespace mrl = mir::report::logging;
namespace mtd = mir::test::doubles;

namespace
{
class MockLogger : public ml::Logger
{
public:
    MOCK_METHOD3(log, void(ml::Severity severity, const std::string& message, const std::string& component));
    ~MockLogger() noexcept(true) {}
};

struct DisplayReport : public testing::Test
{
    std::shared_ptr<mtd::AdvanceableClock> const clock{std::make_shared<mtd::AdvanceableClock>()};
    std::shared_ptr<MockLogger> logger{std::make_shared<MockLogger>()};
    mtd::MockEGL mock_egl;
};

char const* const component = "graphics";

#define STRMACRO(X) #X
std::string egl_string_mapping [] =
{
    STRMACRO(EGL_BUFFER_SIZE),
    STRMACRO(EGL_ALPHA_SIZE),
    STRMACRO(EGL_BLUE_SIZE),
    STRMACRO(EGL_GREEN_SIZE),
    STRMACRO(EGL_RED_SIZE),
    STRMACRO(EGL_DEPTH_SIZE),
    STRMACRO(EGL_STENCIL_SIZE),
    STRMACRO(EGL_CONFIG_CAVEAT),
    STRMACRO(EGL_CONFIG_ID),
    STRMACRO(EGL_LEVEL),
    STRMACRO(EGL_MAX_PBUFFER_HEIGHT),
    STRMACRO(EGL_MAX_PBUFFER_PIXELS),
    STRMACRO(EGL_MAX_PBUFFER_WIDTH),
    STRMACRO(EGL_NATIVE_RENDERABLE),
    STRMACRO(EGL_NATIVE_VISUAL_ID),
    STRMACRO(EGL_NATIVE_VISUAL_TYPE),
    STRMACRO(EGL_SAMPLES),
    STRMACRO(EGL_SAMPLE_BUFFERS),
    STRMACRO(EGL_SURFACE_TYPE),
    STRMACRO(EGL_TRANSPARENT_TYPE),
    STRMACRO(EGL_TRANSPARENT_BLUE_VALUE),
    STRMACRO(EGL_TRANSPARENT_GREEN_VALUE),
    STRMACRO(EGL_TRANSPARENT_RED_VALUE),
    STRMACRO(EGL_BIND_TO_TEXTURE_RGB),
    STRMACRO(EGL_BIND_TO_TEXTURE_RGBA),
    STRMACRO(EGL_MIN_SWAP_INTERVAL),
    STRMACRO(EGL_MAX_SWAP_INTERVAL),
    STRMACRO(EGL_LUMINANCE_SIZE),
    STRMACRO(EGL_ALPHA_MASK_SIZE),
    STRMACRO(EGL_COLOR_BUFFER_TYPE),
    STRMACRO(EGL_RENDERABLE_TYPE),
    STRMACRO(EGL_MATCH_NATIVE_PIXMAP),
    STRMACRO(EGL_CONFORMANT),
    STRMACRO(EGL_SLOW_CONFIG),
    STRMACRO(EGL_NON_CONFORMANT_CONFIG),
    STRMACRO(EGL_TRANSPARENT_RGB),
    STRMACRO(EGL_RGB_BUFFER),
    STRMACRO(EGL_LUMINANCE_BUFFER),
    STRMACRO(EGL_FRAMEBUFFER_TARGET_ANDROID)
};
#undef STRMACRO

}

TEST_F(DisplayReport, eglconfig)
{
    using namespace testing;
    EGLDisplay disp = reinterpret_cast<EGLDisplay>(9);
    EGLConfig config = reinterpret_cast<EGLConfig>(8);
    int dummy_value = 7;
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(disp, config,_,_))
        .Times(AnyNumber())
        .WillRepeatedly(DoAll(SetArgPointee<3>(dummy_value),Return(EGL_TRUE)));

    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        "Display EGL Configuration:",
        component));
    for(auto &i : egl_string_mapping)
    {
        EXPECT_CALL(*logger, log(
            ml::Severity::informational,
            "    [" + i + "] : " + std::to_string(dummy_value),
            component));
    }

    mrl::DisplayReport report(logger, clock);
    report.report_egl_configuration(disp, config);
}

TEST_F(DisplayReport, reports_vsync)
{
    std::chrono::milliseconds interval(1500);
    unsigned int display1_id {1223};
    unsigned int display2_id {4492};
    std::string display1_name(std::to_string(display1_id));
    std::string display2_name(std::to_string(display2_id));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        "2 vsync events on [" + display1_name + "] over " + std::to_string(interval.count()) + "ms",
        component));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        "1 vsync events on [" + display2_name + "] over " + std::to_string(interval.count()) + "ms",
        component));
    mrl::DisplayReport report(logger, clock);

    report.report_vsync(display1_id);
    report.report_vsync(display2_id);
    clock->advance_by(interval);
    report.report_vsync(display1_id);
}
