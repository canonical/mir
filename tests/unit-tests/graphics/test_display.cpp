/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
#include "mir/graphics/gl_context.h"
#include "mir/options/program_option.h"

#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "src/server/graphics/default_display_configuration_policy.h"
#ifndef ANDROID
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "src/server/graphics/gbm/gbm_platform.h"
#include "mir_test_framework/udev_environment.h"
#else
#include "src/server/graphics/android/android_platform.h"
#include "mir_test_doubles/mock_android_hw.h"
#include "mir_test_doubles/mock_display_device.h"
#endif

#include "mir/graphics/null_display_report.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
#ifndef ANDROID
namespace mtf = mir::mir_test_framework;
#endif

class DisplayTest : public ::testing::Test
{
public:
    DisplayTest()
    {
        using namespace testing;

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        const char* egl_exts = "EGL_KHR_image EGL_KHR_image_base EGL_MESA_drm_image";
        const char* gl_exts = "GL_OES_texture_npot GL_OES_EGL_image";

        ON_CALL(mock_egl, eglQueryString(_,EGL_EXTENSIONS))
            .WillByDefault(Return(egl_exts));
        ON_CALL(mock_gl, glGetString(GL_EXTENSIONS))
            .WillByDefault(Return(reinterpret_cast<const GLubyte*>(gl_exts)));

#ifndef ANDROID
        fake_devices.add_standard_drm_devices();
#endif
    }

    std::shared_ptr<mg::Display> create_display()
    {
        auto conf_policy = std::make_shared<mg::DefaultDisplayConfigurationPolicy>();
        auto report = std::make_shared<mg::NullDisplayReport>();
#ifdef ANDROID
        auto platform = mg::create_platform(
            std::make_shared<mir::options::ProgramOption>(),
            report);
#else
        auto platform = std::make_shared<mg::gbm::GBMPlatform>(report,
            std::make_shared<mir::test::doubles::NullVirtualTerminal>());
#endif
        return platform->create_display(conf_policy);
    }

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
#ifdef ANDROID
    ::testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#else
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;
#endif
};

TEST_F(DisplayTest, gl_context_make_current_uses_shared_context)
{
    using namespace testing;
    EGLContext const shared_context{reinterpret_cast<EGLContext>(0x111)};
    EGLContext const display_buffer_context{reinterpret_cast<EGLContext>(0x222)};
    EGLContext const new_context{reinterpret_cast<EGLContext>(0x333)};

    EXPECT_CALL(mock_egl, eglCreateContext(_,_,EGL_NO_CONTEXT,_))
        .WillOnce(Return(shared_context));
    EXPECT_CALL(mock_egl, eglCreateContext(_,_,shared_context,_))
        .WillOnce(Return(display_buffer_context));

    auto display = create_display();

    Mock::VerifyAndClearExpectations(&mock_egl);

    {
        InSequence s;
        EXPECT_CALL(mock_egl, eglCreateContext(_,_,shared_context,_))
            .WillOnce(Return(new_context));
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,new_context));
        EXPECT_CALL(mock_egl, eglGetCurrentContext())
           .WillOnce(Return(new_context));
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));

        auto gl_ctx = display->create_gl_context();

        ASSERT_NE(nullptr, gl_ctx);

        gl_ctx->make_current();
    }

    Mock::VerifyAndClearExpectations(&mock_egl);

    /* Possible display shutdown sequence, depending on the platform */
    EXPECT_CALL(mock_egl, eglGetCurrentContext())
        .Times(AtLeast(0));
    EXPECT_CALL(mock_egl, eglMakeCurrent(_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
        .Times(AtLeast(0));
}

TEST_F(DisplayTest, gl_context_releases_context)
{
    using namespace testing;

    auto display = create_display();

    {
        InSequence s;
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,Ne(EGL_NO_CONTEXT)));
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));

        auto gl_ctx = display->create_gl_context();

        ASSERT_NE(nullptr, gl_ctx);

        gl_ctx->make_current();
        gl_ctx->release_current();

        Mock::VerifyAndClearExpectations(&mock_egl);
    }

    /* Possible display shutdown sequence, depending on the platform */
    EXPECT_CALL(mock_egl, eglMakeCurrent(_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
        .Times(AtLeast(0));
}
