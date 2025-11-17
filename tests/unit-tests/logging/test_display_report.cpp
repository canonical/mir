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

#include "src/server/report/logging/display_report.h"
#include "mir/graphics/frame.h"
#include "mir/logging/logger.h"
#include "mir/test/doubles/mock_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

namespace ml  = mir::logging;
namespace mrl = mir::report::logging;
namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{
class MockLogger : public ml::Logger
{
public:
    MOCK_METHOD(void, log, (ml::Severity severity, const std::string& message, const std::string& component), ());
    ~MockLogger() noexcept(true) {}
};

struct DisplayReport : public testing::Test
{
    std::shared_ptr<MockLogger> logger{std::make_shared<MockLogger>()};
    testing::NiceMock<mtd::MockEGL> mock_egl;
    EGLDisplay disp = reinterpret_cast<EGLDisplay>(9);
    EGLConfig config = reinterpret_cast<EGLConfig>(8);
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
    int dummy_value = 7;
    auto ext_str = "EGL_EXT_imaginary";
    auto client_ext_str = "EGL_EXT_oil_platform";

    EXPECT_CALL(mock_egl, eglQueryString(disp, EGL_EXTENSIONS)).WillOnce(Return(ext_str));
    EXPECT_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)).WillOnce(Return(client_ext_str));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(disp, config,_,_))
        .Times(AnyNumber())
        .WillRepeatedly(DoAll(SetArgPointee<3>(dummy_value),Return(EGL_TRUE)));

    EXPECT_CALL(*logger, log(
        ml::Severity::informational, "Display EGL Extensions: " + std::string{ext_str}, component));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational, "EGL_EXT_client_extensions: " + std::string{client_ext_str}, component));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational, "Display EGL Configuration:", component));
    for(auto &i : egl_string_mapping)
    {
        EXPECT_CALL(*logger, log(
            ml::Severity::informational,
            "    [" + i + "] : " + std::to_string(dummy_value),
            component));
    }

    mrl::DisplayReport report(logger);
    report.report_egl_configuration(disp, config);
}

TEST_F(DisplayReport, eglconfig_clears_eglerror_when_there_are_no_extensions)
{
    EXPECT_CALL(mock_egl, eglQueryString(disp, EGL_EXTENSIONS)).WillOnce(Return(nullptr));
    EXPECT_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)).WillOnce(Return(nullptr));
    EXPECT_CALL(mock_egl, eglGetError());
    mrl::DisplayReport report(logger);
    report.report_egl_configuration(disp, config);
}

TEST_F(DisplayReport, reports_vsync)
{
    std::chrono::nanoseconds const nanos_per_frame{16666666};
    std::string const interval_str{"interval 16.666ms"};
    unsigned int display1_id {1223};
    unsigned int display2_id {4492};
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        AllOf(StartsWith("vsync on "+std::to_string(display1_id)),
              HasSubstr(interval_str)),
        component));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        AllOf(StartsWith("vsync on "+std::to_string(display2_id)),
              HasSubstr(interval_str)),
        component));
    mrl::DisplayReport report(logger);

    mir::graphics::Frame frame;
    report.report_vsync(display1_id, frame);
    report.report_vsync(display2_id, frame);
    frame.msc++;
    frame.ust.nanoseconds += nanos_per_frame;
    report.report_vsync(display1_id, frame);
    report.report_vsync(display2_id, frame);
}

TEST_F(DisplayReport, reports_vsync_steady_interval_despite_missed_frames)
{
    int const hz = 60;
    std::string const interval_str{"interval 16.666ms (60.00Hz)"};
    unsigned const id{123};
    int const d1 = 456;
    int const d2 = 789;

    InSequence seq;

    auto const id_str = std::to_string(id);
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        AllOf(StartsWith("vsync on "+id_str+": #"+std::to_string(d1)+","),
              HasSubstr(interval_str)),
        component));
    EXPECT_CALL(*logger, log(
        ml::Severity::informational,
        AllOf(StartsWith("vsync on "+id_str+": #"+std::to_string(d1+d2)+","),
              HasSubstr(interval_str)),
        component));

    mrl::DisplayReport report(logger);
    mir::graphics::Frame frame;

    std::chrono::nanoseconds const nanos_per_frame{1000000000LL/hz};

    report.report_vsync(id, frame);
    frame.msc += d1;
    frame.ust.nanoseconds += d1 * nanos_per_frame;
    report.report_vsync(id, frame);
    frame.msc += d2;
    frame.ust.nanoseconds += d2 * nanos_per_frame;
    report.report_vsync(id, frame);
}
