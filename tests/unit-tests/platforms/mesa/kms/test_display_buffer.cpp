/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/null_virtual_terminal.h"
#include "src/platforms/mesa/server/kms/platform.h"
#include "src/platforms/mesa/server/kms/display_buffer.h"
#include "src/platforms/mesa/include/native_buffer.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_gbm_native_buffer.h"
#include "mir_test_framework/udev_environment.h"
#include "mir/test/doubles/fake_renderable.h"
#include "mir/graphics/transformation.h"
#include "mock_kms_output.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gbm.h>

using namespace testing;
using namespace mir;
using namespace std;
using namespace mir::test;
using namespace mir::test::doubles;
using namespace mir_test_framework;
using namespace mir::graphics;
using namespace mir::graphics::mesa;
using mir::report::null_display_report;

class MesaDisplayBufferTest : public Test
{
public:
    int const mock_refresh_rate = 60;

    MesaDisplayBufferTest()
        : identity(1)
        , mock_bypassable_buffer{std::make_shared<NiceMock<MockBuffer>>()}
        , mock_software_buffer{std::make_shared<NiceMock<MockBuffer>>()}
        , fake_bypassable_renderable{
             std::make_shared<FakeRenderable>(display_area)}
        , fake_software_renderable{
             std::make_shared<FakeRenderable>(display_area)}
        , stub_gbm_native_buffer{
             std::make_shared<StubGBMNativeBuffer>(display_area.size)}
        , stub_shm_native_buffer{
             std::make_shared<mir::graphics::mesa::NativeBuffer>()}
        , bypassable_list{fake_bypassable_renderable}
    {
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        fake_bo = reinterpret_cast<gbm_bo*>(123);
        ON_CALL(mock_gbm, gbm_surface_lock_front_buffer(_))
            .WillByDefault(Return(fake_bo));
        fake_handle.u32 = 123;
        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
            .WillByDefault(Return(fake_handle));
        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
            .WillByDefault(Return(456));

        fake_devices.add_standard_device("standard-drm-devices");

        mock_kms_output = std::make_shared<NiceMock<MockKMSOutput>>();
        ON_CALL(*mock_kms_output, set_crtc_thunk(_))
            .WillByDefault(Return(true));
        ON_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
            .WillByDefault(Return(true));
        ON_CALL(*mock_kms_output, max_refresh_rate())
            .WillByDefault(Return(mock_refresh_rate));
        ON_CALL(*mock_kms_output, fb_for(_))
            .WillByDefault(Return(reinterpret_cast<FBHandle*>(0x12ad)));
        ON_CALL(*mock_kms_output, buffer_requires_migration(_))
            .WillByDefault(Return(false));

        ON_CALL(*mock_bypassable_buffer, size())
            .WillByDefault(Return(display_area.size));
        ON_CALL(*mock_bypassable_buffer, native_buffer_handle())
            .WillByDefault(Return(stub_gbm_native_buffer));
        fake_bypassable_renderable->set_buffer(mock_bypassable_buffer);

        stub_shm_native_buffer->flags = 0;  // Is not a hardware/GBM buffer

        ON_CALL(*mock_software_buffer, size())
            .WillByDefault(Return(display_area.size));
        ON_CALL(*mock_software_buffer, native_buffer_handle())
            .WillByDefault(Return(stub_shm_native_buffer));
        fake_software_renderable->set_buffer(mock_software_buffer);
    }

protected:
    GBMOutputSurface make_output_surface()
    {
        helpers::EGLHelper egl{gl_config};
        return GBMOutputSurface{
            drm->fd,
            GBMSurfaceUPtr{nullptr},
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            std::move(egl)
        };
    }

    int const width{56};
    int const height{78};
    mir::geometry::Rectangle const display_area{{12,34}, {width,height}};
    glm::mat2 const identity;
    NiceMock<MockGBM> mock_gbm;
    NiceMock<MockEGL> mock_egl;
    NiceMock<MockGL>  mock_gl;
    NiceMock<MockDRM> mock_drm; 
    std::shared_ptr<MockBuffer> mock_bypassable_buffer;
    std::shared_ptr<MockBuffer> mock_software_buffer;
    std::shared_ptr<FakeRenderable> fake_bypassable_renderable;
    std::shared_ptr<FakeRenderable> fake_software_renderable;
    std::shared_ptr<mir::graphics::mesa::NativeBuffer> stub_gbm_native_buffer;
    std::shared_ptr<mir::graphics::mesa::NativeBuffer> stub_shm_native_buffer;
    gbm_bo*           fake_bo;
    gbm_bo_handle     fake_handle;
    UdevEnvironment   fake_devices;
    std::shared_ptr<MockKMSOutput> mock_kms_output;
    StubGLConfig gl_config;
    mir::graphics::RenderableList const bypassable_list;
    std::shared_ptr<helpers::GBMHelper> gbm{std::make_shared<helpers::GBMHelper>()};
    std::shared_ptr<helpers::DRMHelper> drm{std::make_shared<helpers::DRMHelper>(helpers::DRMNodeToUse::card)};
};

TEST_F(MesaDisplayBufferTest, unrotated_view_area_is_untouched)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_EQ(display_area, db.view_area());
}

TEST_F(MesaDisplayBufferTest, bypass_buffer_is_held_for_full_frame)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    auto original_count = mock_bypassable_buffer.use_count();

    EXPECT_TRUE(db.overlay(bypassable_list));
    EXPECT_EQ(original_count+1, mock_bypassable_buffer.use_count());

    // Switch back to normal compositing
    db.make_current();
    db.swap_buffers();
    db.post();

    // Bypass buffer should no longer be held by db
    EXPECT_EQ(original_count, mock_bypassable_buffer.use_count());
}

TEST_F(MesaDisplayBufferTest, predictive_bypass_is_throttled)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    for (int frame = 0; frame < 5; ++frame)
    {
        ASSERT_TRUE(db.overlay(bypassable_list));
        db.post();

        // Cast to a simple int type so that test failures are readable
        int milliseconds_per_frame = 1000 / mock_refresh_rate;
        ASSERT_THAT(db.recommended_sleep().count(),
                    Ge(milliseconds_per_frame/2));
    }
}

TEST_F(MesaDisplayBufferTest, frames_requiring_gl_are_not_throttled)
{
    graphics::RenderableList non_bypassable_list{
        std::make_shared<FakeRenderable>(geometry::Rectangle{{12, 34}, {1, 1}})
    };

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    for (int frame = 0; frame < 5; ++frame)
    {
        ASSERT_FALSE(db.overlay(non_bypassable_list));
        db.post();

        // Cast to a simple int type so that test failures are readable
        ASSERT_EQ(0, db.recommended_sleep().count());
    }
}

TEST_F(MesaDisplayBufferTest, bypass_buffer_only_referenced_once_by_db)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    auto original_count = mock_bypassable_buffer.use_count();

    EXPECT_TRUE(db.overlay(bypassable_list));
    EXPECT_EQ(original_count+1, mock_bypassable_buffer.use_count());

    db.post();

    // Bypass buffer still held by DB only one ref above the original
    EXPECT_EQ(original_count+1, mock_bypassable_buffer.use_count());
}

TEST_F(MesaDisplayBufferTest, untransformed_with_bypassable_list_can_bypass)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_TRUE(db.overlay(bypassable_list));
}

TEST_F(MesaDisplayBufferTest, failed_bypass_falls_back_gracefully)
{  // Regression test for LP: #1398296
    EXPECT_CALL(*mock_kms_output, fb_for(_))
        .WillOnce(Return(reinterpret_cast<FBHandle*>(0xaabb)))  // During the DisplayBuffer constructor
        .WillOnce(Return(nullptr)) // Fail first bypass attempt
        .WillOnce(Return(reinterpret_cast<FBHandle*>(0xbbcc))); // Succeed second bypass attempt

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(bypassable_list));
    // And then we recover. DRM finds enough resources to AddFB ...
    EXPECT_TRUE(db.overlay(bypassable_list));
}

TEST_F(MesaDisplayBufferTest, skips_bypass_because_of_lagging_resize)
{  // Another regression test for LP: #1398296
    auto fullscreen = std::make_shared<FakeRenderable>(display_area);
    auto nonbypassable = std::make_shared<testing::NiceMock<MockBuffer>>();
    ON_CALL(*nonbypassable, native_buffer_handle())
        .WillByDefault(Return(stub_gbm_native_buffer));
    ON_CALL(*nonbypassable, size())
        .WillByDefault(Return(mir::geometry::Size{12,34}));

    fullscreen->set_buffer(nonbypassable);
    graphics::RenderableList list{fullscreen};

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(list));
}

TEST_F(MesaDisplayBufferTest, rotated_cannot_bypass)
{
    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        transformation(mir_orientation_right));

    EXPECT_FALSE(db.overlay(bypassable_list));
}

TEST_F(MesaDisplayBufferTest, fullscreen_software_buffer_cannot_bypass)
{
    graphics::RenderableList const list{fake_software_renderable};

    // Passes the bypass candidate test:
    EXPECT_EQ(fake_software_renderable->buffer()->size(), display_area.size);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(list));
}

TEST_F(MesaDisplayBufferTest, fullscreen_software_buffer_not_used_as_gbm_bo)
{   // Also checks it doesn't crash (LP: #1493721)
    graphics::RenderableList const list{fake_software_renderable};

    // Passes the bypass candidate test:
    EXPECT_EQ(fake_software_renderable->buffer()->size(), display_area.size);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    // If you find yourself using gbm_ functions on a Shm buffer then you're
    // asking for a crash (LP: #1493721) ...
    EXPECT_CALL(mock_gbm, gbm_bo_get_user_data(_)).Times(0);
    db.overlay(list);
}

TEST_F(MesaDisplayBufferTest, transformation_not_implemented_internally)
{
    glm::mat2 const rotate_left = transformation(mir_orientation_left);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        rotate_left);

    EXPECT_EQ(rotate_left, db.transformation());
}

TEST_F(MesaDisplayBufferTest, clone_mode_first_flip_flips_but_no_wait)
{
    // Ensure clone mode can do multiple page flips in parallel without
    // blocking on either (at least till the second post)
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .Times(2);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output, mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    db.swap_buffers();
    db.post();
}

TEST_F(MesaDisplayBufferTest, single_mode_first_post_flips_with_wait)
{
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .Times(1);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(1);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    db.swap_buffers();
    db.post();
}

TEST_F(MesaDisplayBufferTest, clone_mode_waits_for_page_flip_on_second_flip)
{
    InSequence seq;

    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .Times(2);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(2);
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .Times(2);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output, mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    db.swap_buffers();
    db.post();

    db.swap_buffers();
    db.post();
}

TEST_F(MesaDisplayBufferTest, skips_bypass_because_of_incompatible_list)
{
    graphics::RenderableList list{
        std::make_shared<FakeRenderable>(display_area),
        std::make_shared<FakeRenderable>(geometry::Rectangle{{12, 34}, {1, 1}})
    };

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(list));
}

TEST_F(MesaDisplayBufferTest, skips_bypass_because_of_incompatible_bypass_buffer)
{
    auto fullscreen = std::make_shared<FakeRenderable>(display_area);
    auto nonbypassable = std::make_shared<testing::NiceMock<MockBuffer>>();
    auto nonbypassable_gbm_native_buffer =
        std::make_shared<StubGBMNativeBuffer>(display_area.size, false);
    ON_CALL(*nonbypassable, native_buffer_handle())
        .WillByDefault(Return(nonbypassable_gbm_native_buffer));
    ON_CALL(*nonbypassable, size())
        .WillByDefault(Return(display_area.size));

    fullscreen->set_buffer(nonbypassable);
    graphics::RenderableList list{fullscreen};

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(list));
}

TEST_F(MesaDisplayBufferTest, buffer_requiring_migration_is_ineligable_for_bypass)
{
    ON_CALL(*mock_kms_output, buffer_requires_migration(Eq(stub_gbm_native_buffer->bo)))
        .WillByDefault(Return(true));

    graphics::mesa::DisplayBuffer db(
        graphics::mesa::BypassOption::allowed,
        null_display_report(),
        {mock_kms_output},
        make_output_surface(),
        display_area,
        identity);

    EXPECT_FALSE(db.overlay(bypassable_list));
}
