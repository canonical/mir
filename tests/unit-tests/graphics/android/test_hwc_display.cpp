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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/graphics/android/display_buffer.h"
#include "src/server/graphics/android/android_display.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_display_device.h"
#include <memory>

namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class AndroidDisplayBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_display_device = std::make_shared<mtd::MockDisplayDevice>();
        native_window = std::make_shared<mg::android::MirNativeWindow>(std::make_shared<mtd::StubDriverInterpreter>());

        visual_id = 5;
        dummy_display = reinterpret_cast<EGLDisplay>(0x34);
        dummy_config = reinterpret_cast<EGLConfig>(0x44);
        dummy_context = reinterpret_cast<EGLContext>(0x58);
    }

    int visual_id;
    EGLConfig dummy_config;
    EGLDisplay dummy_display;
    EGLContext dummy_context;

    std::shared_ptr<ANativeWindow> native_window;
    std::shared_ptr<mtd::MockDisplayDevice> mock_display_device;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(AndroidDisplayBufferTest, test_post_submits_right_egl_parameters)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_display_device, display_size())
        .Times(AnyNumber())
        .WillRepeatedly(Return(fake_display_size)); 

    mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);

    EXPECT_CALL(*mock_display_device, commit_frame(dummy_display, mock_egl.fake_egl_surface))
        .Times(1);

    db.post_update();
}

TEST_F(AndroidDisplayBufferTest, test_db_forwards_size_along)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_display_device, display_size())
        .Times(AnyNumber())
        .WillRepeatedly(Return(fake_display_size));
 
    mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
    
    auto view_area = db.view_area();

    geom::Point origin_pt{geom::X{0}, geom::Y{0}};
    EXPECT_EQ(view_area.size, fake_display_size);
    EXPECT_EQ(view_area.top_left, origin_pt);
}

TEST_F(AndroidDisplayBufferTest, db_egl_context_from_shared)
{
    using namespace testing;

    EGLint const expected_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    InSequence seq;
    EXPECT_CALL(mock_egl, eglCreateContext(
        dummy_display, dummy_config, dummy_context, mtd::AttrMatches(expected_attr)))
        .Times(1)
        .WillOnce(Return(mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(
        dummy_display, dummy_config, native_window.get(), NULL))
        .Times(1)
        .WillOnce(Return(mock_egl.fake_egl_surface));
    EXPECT_CALL(mock_egl, eglDestroySurface(dummy_display, mock_egl.fake_egl_surface))
        .Times(1);
    EXPECT_CALL(mock_egl, eglDestroyContext(dummy_display, mock_egl.fake_egl_context))
        .Times(1);

    mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
}

TEST_F(AndroidDisplayBufferTest, egl_resource_creation_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .Times(2)
        .WillOnce(Return(EGL_NO_CONTEXT))
        .WillOnce(Return(mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(_,_,_,_))
        .Times(1)
        .WillOnce(Return(EGL_NO_SURFACE));

    EXPECT_THROW(
    {
        mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
    }, std::runtime_error);

    EXPECT_THROW(
    {
        mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
    }, std::runtime_error);
}

TEST_F(AndroidDisplayBufferTest, make_current)
{
    using namespace testing;
    EGLContext fake_ctxt = reinterpret_cast<EGLContext>(0x4422);
    EGLSurface fake_surf = reinterpret_cast<EGLSurface>(0x33984);

    EXPECT_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .Times(1)
        .WillOnce(Return(fake_ctxt));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(_,_,_,_))
        .Times(1)
        .WillOnce(Return(fake_surf));

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, fake_surf, fake_surf, fake_ctxt))
        .Times(2)
        .WillOnce(Return(EGL_TRUE))
        .WillOnce(Return(EGL_FALSE));

    mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
    db.make_current();
    EXPECT_THROW(
    {
        db.make_current();
    }, std::runtime_error);
}

TEST_F(AndroidDisplayBufferTest, release_current)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        .Times(2)
        .WillOnce(Return(EGL_TRUE))
        .WillOnce(Return(EGL_FALSE));

    mga::DisplayBuffer db(mock_display_device, native_window, dummy_display, dummy_config, dummy_context);
    db.release_current();
    EXPECT_THROW(
    {
        db.release_current();
    }, std::runtime_error);
}
#if 0
=======
TEST_F(AndroidTestHWCFramebuffer, test_dpms_configuration_changes_reach_device)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_display_device, display_size())
        .Times(1)
        .WillOnce(Return(fake_display_size)); 
    auto display = create_display();
    
    auto on_configuration = display->configuration();
    on_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        on_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_on);
    });
    auto off_configuration = display->configuration();
    off_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        off_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_off);
    });
    auto standby_configuration = display->configuration();
    standby_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        standby_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_standby);
    });
    auto suspend_configuration = display->configuration();
    suspend_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        suspend_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_suspend);
    });

    {
        InSequence seq;
        EXPECT_CALL(*mock_display_device, mode(mir_power_mode_on));
        EXPECT_CALL(*mock_display_device, mode(mir_power_mode_off));
        EXPECT_CALL(*mock_display_device, mode(mir_power_mode_suspend));
        EXPECT_CALL(*mock_display_device, mode(mir_power_mode_standby));
    }
    display->configure(*on_configuration);
    display->configure(*off_configuration);
    display->configure(*suspend_configuration);
    display->configure(*standby_configuration);
>>>>>>> MERGE-SOURCE
#endif
