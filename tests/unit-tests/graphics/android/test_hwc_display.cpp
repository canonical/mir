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
#include "src/server/graphics/android/gl_context.h"
#include "src/server/graphics/android/android_format_conversion-inl.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_display_device.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
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
        stub_buffer = std::make_shared<testing::NiceMock<mtd::StubBuffer>>();
        mock_display_device = std::make_shared<mtd::MockDisplayDevice>();
        native_window = std::make_shared<mg::android::MirNativeWindow>(std::make_shared<mtd::StubDriverInterpreter>());

        visual_id = 5;
        dummy_display = mock_egl.fake_egl_display;
        dummy_config = mock_egl.fake_configs[0];
        dummy_context = mock_egl.fake_egl_context;
        mtd::MockDisplayReport report;
        gl_context = std::make_shared<mga::GLContext>(mga::to_mir_format(mock_egl.fake_visual_id),report);
        mock_fb_bundle = std::make_shared<mtd::MockFBBundle>();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;

    int visual_id;
    EGLConfig dummy_config;
    EGLDisplay dummy_display;
    EGLContext dummy_context;
    std::shared_ptr<mga::GLContext> gl_context;

    std::shared_ptr<mtd::StubBuffer> stub_buffer;
    std::shared_ptr<ANativeWindow> native_window;
    std::shared_ptr<mtd::MockDisplayDevice> mock_display_device;
    std::shared_ptr<mtd::MockFBBundle> mock_fb_bundle;
};

TEST_F(AndroidDisplayBufferTest, test_post_update)
{
    using namespace testing;

    mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);

    InSequence seq;
    EXPECT_CALL(*mock_display_device, prepare_composition())
        .Times(1);
    EXPECT_CALL(*mock_display_device, gpu_render(dummy_display, mock_egl.fake_egl_surface))
        .Times(1);
    EXPECT_CALL(*mock_fb_bundle, last_rendered_buffer())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_display_device, post(Ref(*stub_buffer)))
        .Times(1);

    db.post_update();
}

TEST_F(AndroidDisplayBufferTest, test_db_forwards_size_along)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_fb_bundle, fb_size())
        .Times(AnyNumber())
        .WillRepeatedly(Return(fake_display_size));
 
    mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
    
    auto view_area = db.view_area();

    geom::Point origin_pt{geom::X{0}, geom::Y{0}};
    EXPECT_EQ(view_area.size, fake_display_size);
    EXPECT_EQ(view_area.top_left, origin_pt);
}

TEST_F(AndroidDisplayBufferTest, db_egl_context_from_shared)
{
    using namespace testing;

    EGLint const expected_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    EXPECT_CALL(mock_egl, eglCreateContext(
        dummy_display, _, dummy_context, mtd::AttrMatches(expected_attr)))
        .Times(1)
        .WillOnce(Return(mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(
        dummy_display, _, native_window.get(), NULL))
        .Times(1)
        .WillOnce(Return(mock_egl.fake_egl_surface));
    EXPECT_CALL(mock_egl, eglDestroySurface(dummy_display, mock_egl.fake_egl_surface))
        .Times(AtLeast(1));
    EXPECT_CALL(mock_egl, eglDestroyContext(dummy_display, mock_egl.fake_egl_context))
        .Times(AtLeast(1));

    mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
    testing::Mock::VerifyAndClearExpectations(&mock_egl);
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
        mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
    }, std::runtime_error);

    EXPECT_THROW(
    {
        mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
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

    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, fake_surf, fake_surf, fake_ctxt))
        .Times(2)
        .WillOnce(Return(EGL_TRUE))
        .WillOnce(Return(EGL_FALSE));

    mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
    db.make_current();
    EXPECT_THROW(
    {
        db.make_current();
    }, std::runtime_error);
}

TEST_F(AndroidDisplayBufferTest, release_current)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        .Times(1);

    mga::DisplayBuffer db(mock_fb_bundle, mock_display_device, native_window, *gl_context);
    db.release_current();
}
