/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/platform.h"
#include "mir/options/program_option.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "mir/test/doubles/null_console_services.h"
#include "src/server/report/null/display_report.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#include "src/platforms/mesa/server/x11/graphics/platform.h"
#include "mir/test/doubles/mock_x11.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

class DisplayTestGeneric : public ::testing::Test
{
public:
    DisplayTestGeneric()
    {
        using namespace testing;

        EGLint const client_version = 2;

        ON_CALL(mock_egl, eglQueryContext(mock_egl.fake_egl_display,
                                          mock_egl.fake_egl_context,
                                          EGL_CONTEXT_CLIENT_VERSION,
                                          _))
            .WillByDefault(DoAll(SetArgPointee<3>(client_version),
                            Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display,
                                          mock_egl.fake_egl_surface,
                                          EGL_WIDTH,
                                          _))
            .WillByDefault(DoAll(SetArgPointee<3>(1280),
                            Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display,
                                          mock_egl.fake_egl_surface,
                                          EGL_HEIGHT,
                                          _))
            .WillByDefault(DoAll(SetArgPointee<3>(1024),
                            Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display,
                                             _,
                                             _,
                                             _))
            .WillByDefault(DoAll(SetArgPointee<3>(EGL_WINDOW_BIT),
                            Return(EGL_TRUE)));

        ON_CALL(mock_x11, XNextEvent(mock_x11.fake_x11.display,
                                     _))
            .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.expose_event_return),
                       Return(1)));

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        fake_devices.add_standard_device("standard-drm-render-nodes");
    }

    std::shared_ptr<mg::Display> create_display()
    {
        auto const platform = std::make_shared<mg::X::Platform>(
            std::shared_ptr<::Display>(
                XOpenDisplay(nullptr),
                [](::Display* display)
                {
                    XCloseDisplay(display);
                }),
            std::vector<mg::X::X11OutputConfig>{{{1280, 1024}}},
            std::make_shared<mir::report::null::DisplayReport>());
        return platform->create_display(
            std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
            std::make_shared<mtd::StubGLConfig>());
    }

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;
    ::testing::NiceMock<mtd::MockX11> mock_x11;
};

#define MIR_DISABLE_TESTS_ON_X11
#include "../../test_display.h"
#undef MIR_DISABLE_TESTS_ON_X11
