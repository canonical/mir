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
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/platform.h"
#include "mir/options/program_option.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_gl_program_factory.h"
#include "mir/test/doubles/platform_factory.h"
#include "mir/graphics/default_display_configuration_policy.h"
#if defined(MESA_KMS) || defined(MESA_X11)
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#elif ANDROID
#include "mir/test/doubles/mock_android_hw.h"
#include "mir/test/doubles/mock_display_device.h"
#endif
#ifdef MESA_X11
#include "mir/test/doubles/mock_x11.h"
#endif

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
#if defined(MESA_KMS) || defined(MESA_X11)
namespace mtf = mir_test_framework;
#endif

class DisplayTest : public ::testing::Test
{
public:
    DisplayTest()
    {
        using namespace testing;

#ifdef MESA_X11
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
#endif

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

#ifdef MESA_KMS
        fake_devices.add_standard_device("standard-drm-devices");
#elif MESA_X11
        fake_devices.add_standard_device("standard-drm-render-nodes");
#endif
    }

    std::shared_ptr<mg::Display> create_display()
    {
        auto const platform = mtd::create_platform_with_null_dependencies();
        return platform->create_display(
            std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
            std::make_shared<mtd::StubGLProgramFactory>(),
            std::make_shared<mtd::StubGLConfig>());
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
#ifdef MESA_X11
    ::testing::NiceMock<mtd::MockX11> mock_x11;
#endif
};

namespace
{

class MockDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    MOCK_CONST_METHOD1(for_each_card,
        void(std::function<void(mg::DisplayConfigurationCard const&)>));
    MOCK_CONST_METHOD1(for_each_output,
        void(std::function<void(mg::DisplayConfigurationOutput const&)>));
    MOCK_METHOD1(for_each_output,
        void(std::function<void(mg::UserDisplayConfigurationOutput&)>));
    MOCK_CONST_METHOD0(valid, bool());
};

}

TEST_F(DisplayTest, configure_disallows_invalid_configuration)
{
    using namespace testing;
    auto display = create_display();
    MockDisplayConfiguration config;

    EXPECT_CALL(config, valid())
        .WillOnce(Return(false));

    EXPECT_THROW({display->configure(config);}, std::logic_error);

    // Determining what counts as a valid configuration is a much trickier
    // platform-dependent exercise, so won't be tested here.
}

#ifdef MESA_X11
TEST_F(DisplayTest, DISABLED_gl_context_make_current_uses_shared_context)
#else
TEST_F(DisplayTest, gl_context_make_current_uses_shared_context)
#endif
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

#ifdef MESA_X11
TEST_F(DisplayTest, DISABLED_does_not_expose_display_buffer_for_output_with_power_mode_off)
#else
TEST_F(DisplayTest, does_not_expose_display_buffer_for_output_with_power_mode_off)
#endif
{
    using namespace testing;
    auto display = create_display();
    int db_count{0};

    display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([&] (mg::DisplayBuffer&) { ++db_count; });
    });
    EXPECT_THAT(db_count, Eq(1));

    auto conf = display->configuration();
    conf->for_each_output(
        [] (mg::UserDisplayConfigurationOutput& output)
        {
            output.power_mode = mir_power_mode_off;
        });

    display->configure(*conf);

    db_count = 0;
    display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([&] (mg::DisplayBuffer&) { ++db_count; });
    });
    EXPECT_THAT(db_count, Eq(0));
}
