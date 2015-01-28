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

#include "src/platforms/android/server/display_buffer.h"
#include "src/platforms/android/server/gl_context.h"
#include "src/platforms/android/server/android_format_conversion-inl.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_gl_program_factory.h"
#include <memory>

namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

namespace
{
struct DisplayBuffer : public ::testing::Test
{
    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;
    mtd::StubGLProgramFactory stub_program_factory;

    int visual_id{5};
    EGLConfig dummy_config{mock_egl.fake_configs[0]};
    EGLDisplay dummy_display{mock_egl.fake_egl_display};
    EGLContext dummy_context{mock_egl.fake_egl_context};
    mtd::StubGLConfig stub_gl_config;
    testing::NiceMock<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<mga::GLContext> gl_context{
        std::make_shared<mga::PbufferGLContext>(
            mga::to_mir_format(mock_egl.fake_visual_id), stub_gl_config, mock_display_report)};
    std::shared_ptr<mtd::StubBuffer> stub_buffer{
        std::make_shared<testing::NiceMock<mtd::StubBuffer>>()};
    std::shared_ptr<ANativeWindow> native_window{
        std::make_shared<mg::android::MirNativeWindow>(
            std::make_shared<mtd::StubDriverInterpreter>())};
    std::shared_ptr<mtd::MockDisplayDevice> mock_display_device{
        std::make_shared<testing::NiceMock<mtd::MockDisplayDevice>>()};
    geom::Size const display_size{433,232};
    double const refresh_rate{60.0};
    std::shared_ptr<mtd::MockFBBundle> mock_fb_bundle{
        std::make_shared<testing::NiceMock<mtd::MockFBBundle>>(
            display_size, refresh_rate, mir_pixel_format_abgr_8888)};
    MirOrientation orientation{mir_orientation_normal};
    mga::DisplayBuffer db{
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        mga::OverlayOptimization::enabled};
};
}

TEST_F(DisplayBuffer, can_post_update_with_gl_only)
{
    EXPECT_CALL(*mock_display_device, post_gl(testing::_));
    db.gl_swap_buffers();
    db.flip();
}

TEST_F(DisplayBuffer, posts_overlay_list_returns_display_device_decision)
{
    using namespace testing;
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()};

    EXPECT_CALL(*mock_display_device, post_overlays(_, Ref(renderlist), _))
        .Times(2)
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}

TEST_F(DisplayBuffer, rotation_transposes_dimensions_and_reports_correctly)
{
    geom::Size const transposed{display_size.height.as_int(), display_size.width.as_int()};
    mga::DisplayBuffer inv_db(
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        mir_orientation_inverted,
        mga::OverlayOptimization::enabled);
    mga::DisplayBuffer left_db(
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        mir_orientation_left,
        mga::OverlayOptimization::enabled);
    mga::DisplayBuffer right_db(
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        mir_orientation_right,
        mga::OverlayOptimization::enabled);

    EXPECT_EQ(display_size, db.view_area().size);
    EXPECT_EQ(db.orientation(), mir_orientation_normal);

    EXPECT_EQ(display_size, inv_db.view_area().size);
    EXPECT_EQ(inv_db.orientation(), mir_orientation_inverted);

    EXPECT_EQ(transposed, left_db.view_area().size);
    EXPECT_EQ(left_db.orientation(), mir_orientation_left);

    EXPECT_EQ(transposed, right_db.view_area().size);
    EXPECT_EQ(right_db.orientation(), mir_orientation_right);
}

TEST_F(DisplayBuffer, reports_correct_size)
{
    auto view_area = db.view_area();
    geom::Point origin_pt{geom::X{0}, geom::Y{0}};
    EXPECT_EQ(display_size, view_area.size);
    EXPECT_EQ(origin_pt, view_area.top_left);
}

TEST_F(DisplayBuffer, creates_egl_context_from_shared_context)
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

    mga::DisplayBuffer db{
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        mga::OverlayOptimization::enabled};
    testing::Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(DisplayBuffer, fails_on_egl_resource_creation)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .Times(2)
        .WillOnce(Return(EGL_NO_CONTEXT))
        .WillOnce(Return(mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(_,_,_,_))
        .Times(1)
        .WillOnce(Return(EGL_NO_SURFACE));

    EXPECT_THROW({
        mga::DisplayBuffer db(
            mock_fb_bundle,
            mock_display_device,
            native_window,
            *gl_context,
            stub_program_factory,
            orientation,
            mga::OverlayOptimization::enabled);
    }, std::runtime_error);
    EXPECT_THROW({
        mga::DisplayBuffer db(
            mock_fb_bundle,
            mock_display_device,
            native_window,
            *gl_context,
            stub_program_factory,
            orientation,
            mga::OverlayOptimization::enabled);
    }, std::runtime_error);
}

TEST_F(DisplayBuffer, can_make_current)
{
    EXPECT_CALL(mock_egl, eglMakeCurrent(
        dummy_display, mock_egl.fake_egl_surface, mock_egl.fake_egl_surface, dummy_context))
        .Times(2)
        .WillOnce(testing::Return(EGL_TRUE))
        .WillOnce(testing::Return(EGL_FALSE));

    db.make_current();
    EXPECT_THROW({
        db.make_current();
    }, std::runtime_error);
}

TEST_F(DisplayBuffer, release_current)
{
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    db.release_current();
}

TEST_F(DisplayBuffer, notifies_list_that_content_is_cleared)
{
    EXPECT_CALL(*mock_display_device, content_cleared())
        .Times(3);
    db.configure(mir_power_mode_off, mir_orientation_normal);
    db.configure(mir_power_mode_suspend, mir_orientation_normal);
    db.configure(mir_power_mode_standby, mir_orientation_normal);
    db.configure(mir_power_mode_on, mir_orientation_normal);
}

TEST_F(DisplayBuffer, does_not_use_alpha)
{
    EXPECT_FALSE(db.uses_alpha());
}

TEST_F(DisplayBuffer, reject_list_if_option_disabled)
{
    mg::RenderableList renderlist{std::make_shared<mtd::StubRenderable>()};
    mga::DisplayBuffer db(
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        mga::OverlayOptimization::disabled);

    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}
