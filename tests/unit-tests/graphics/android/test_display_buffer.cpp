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
#include "src/include/common/mir/graphics/android/android_format_conversion-inl.h"
#include "mir/test/doubles/mock_display_device.h"
#include "mir/test/doubles/mock_display_report.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir/test/doubles/stub_driver_interpreter.h"
#include "mir/test/doubles/stub_display_buffer.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/mock_framebuffer_bundle.h"
#include "mir/test/doubles/stub_gl_program_factory.h"
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
    geom::Displacement top_left{0,0};
    std::unique_ptr<mga::LayerList> list{
        new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)};
    std::shared_ptr<mtd::MockFBBundle> mock_fb_bundle{
        std::make_shared<testing::NiceMock<mtd::MockFBBundle>>(display_size)};
    MirOrientation orientation{mir_orientation_normal};
    mga::DisplayBuffer db{
        mga::DisplayName::primary,
        std::unique_ptr<mga::LayerList>(
            new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)),
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        top_left,
        mga::OverlayOptimization::enabled};

};
}

TEST_F(DisplayBuffer, posts_overlay_list_returns_display_device_decision)
{
    using namespace testing;
    mg::RenderableList renderlist{
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()};

    EXPECT_CALL(*mock_display_device, compatible_renderlist(Ref(renderlist)))
        .Times(2)
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}

TEST_F(DisplayBuffer, defaults_to_normal_orientation)
{
    EXPECT_EQ(mir_orientation_normal, db.orientation());
}

TEST_F(DisplayBuffer, rotation_transposes_dimensions_and_reports_correctly)
{
    geom::Size const transposed{display_size.height.as_int(), display_size.width.as_int()};
    EXPECT_EQ(display_size, db.view_area().size);
    EXPECT_EQ(db.orientation(), mir_orientation_normal);
    db.configure(mir_power_mode_on, mir_orientation_inverted, top_left);

    EXPECT_EQ(display_size, db.view_area().size);
    EXPECT_EQ(db.orientation(), mir_orientation_inverted);
    db.configure(mir_power_mode_on, mir_orientation_left, top_left);

    EXPECT_EQ(transposed, db.view_area().size);
    EXPECT_EQ(db.orientation(), mir_orientation_left);
    db.configure(mir_power_mode_on, mir_orientation_right, top_left);

    EXPECT_EQ(transposed, db.view_area().size);
    EXPECT_EQ(db.orientation(), mir_orientation_right);
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
    testing::Mock::VerifyAndClearExpectations(&mock_egl);

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

    {
    mga::DisplayBuffer db{
        mga::DisplayName::primary,
        std::unique_ptr<mga::LayerList>(
            new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)),
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        top_left,
        mga::OverlayOptimization::enabled};
    }
    
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
            mga::DisplayName::primary,
            std::unique_ptr<mga::LayerList>(
                new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)),
            mock_fb_bundle,
            mock_display_device,
            native_window,
            *gl_context,
            stub_program_factory,
            orientation,
            top_left,
            mga::OverlayOptimization::enabled);
    }, std::runtime_error);

    EXPECT_THROW({
        mga::DisplayBuffer db(
            mga::DisplayName::primary,
            std::unique_ptr<mga::LayerList>(
                new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)),
            mock_fb_bundle,
            mock_display_device,
            native_window,
            *gl_context,
            stub_program_factory,
            orientation,
            top_left,
            mga::OverlayOptimization::enabled);
    }, std::runtime_error);
}

TEST_F(DisplayBuffer, can_make_current)
{
    using namespace testing;
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
    db.configure(mir_power_mode_off, mir_orientation_normal, top_left);
    db.configure(mir_power_mode_suspend, mir_orientation_normal, top_left);
    db.configure(mir_power_mode_standby, mir_orientation_normal, top_left);
    db.configure(mir_power_mode_on, mir_orientation_normal, top_left);
}

TEST_F(DisplayBuffer, reject_list_if_option_disabled)
{
    using namespace testing;
    ON_CALL(*mock_display_device, compatible_renderlist(_))
        .WillByDefault(Return(true));

    mg::RenderableList renderlist{std::make_shared<mtd::StubRenderable>()};
    mga::DisplayBuffer db(
        mga::DisplayName::primary,
        std::unique_ptr<mga::LayerList>(
            new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, top_left)),
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        top_left,
        mga::OverlayOptimization::disabled);

    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}

TEST_F(DisplayBuffer, rejects_commit_if_list_doesnt_need_commit)
{
    using namespace testing;
    auto buffer1 = std::make_shared<mtd::StubRenderable>();
    auto buffer2 = std::make_shared<mtd::StubRenderable>();
    auto buffer3 = std::make_shared<mtd::StubRenderable>();

    ON_CALL(*mock_display_device, compatible_renderlist(_))
        .WillByDefault(Return(true));
    auto set_to_overlays = [](mga::LayerList& list)
    {
        auto native_list = list.native_list();
        for (auto i = 0u; i < native_list->numHwLayers; i++)
        {
            if (native_list->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
                native_list->hwLayers[i].compositionType = HWC_OVERLAY;
        }
    };

    mg::RenderableList renderlist{buffer1, buffer2};
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist));
    set_to_overlays(db.contents().list);
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 

    renderlist = mg::RenderableList{buffer2, buffer1}; //ordering changed
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    set_to_overlays(db.contents().list);
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 

    renderlist = mg::RenderableList{buffer3, buffer1}; //buffer changed
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    set_to_overlays(db.contents().list);
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}

TEST_F(DisplayBuffer, reports_position_correctly)
{
    using namespace testing;
    geom::Point origin;
    geom::Displacement offset{100, 100};

    EXPECT_THAT(db.view_area().top_left, Eq(origin));
    db.configure(mir_power_mode_on, orientation, offset);
    EXPECT_THAT(db.view_area().top_left, Eq(geom::Point{origin + offset}));
}

//lp: #1485070. Could alternitvely rotate all the renderables, once rotation is supported
TEST_F(DisplayBuffer, rejects_lists_if_db_is_rotated)
{
    ON_CALL(*mock_display_device, compatible_renderlist(testing::_))
        .WillByDefault(testing::Return(true));
    mg::RenderableList const renderlist{
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()};

    db.configure(mir_power_mode_on, mir_orientation_inverted, geom::Displacement{0,0});
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist));
    db.configure(mir_power_mode_on, mir_orientation_normal, geom::Displacement{0,0});
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist));
}
