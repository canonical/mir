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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/graphics/nested/display.h"
#include "src/server/graphics/nested/host_connection.h"
#include "src/server/graphics/nested/host_surface.h"
#include "src/server/report/null/display_report.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "src/server/input/null_input_dispatcher.h"
#include "mir_display_configuration_builder.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl_config.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/doubles/stub_cursor_listener.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/fake_shared.h"
#include "mir/test/display_config_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mt = mir::test;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;

namespace
{

class SingleDisplayHostConnection : public mtd::StubHostConnection
{
public:
    std::shared_ptr<MirDisplayConfig> create_display_config() override
    {
        return mt::build_trivial_configuration();
    }

    std::vector<mir::ExtensionDescription> extensions() const override { return {}; }
};

class MockApplyDisplayConfigHostConnection : public SingleDisplayHostConnection
{
public:
    MOCK_METHOD1(apply_display_config, void(MirDisplayConfig&));
};

struct NestedDisplay : testing::Test
{
    std::unique_ptr<mgn::Display> create_nested_display(
        std::shared_ptr<mg::Platform> const& platform,
        std::shared_ptr<mg::GLConfig> const& gl_config)
    {
        auto nested_display_raw = new mgn::Display{
            platform,
            std::make_shared<SingleDisplayHostConnection>(),
            mt::fake_shared(null_display_report),
            mt::fake_shared(default_conf_policy),
            gl_config,
            mgn::PassthroughOption::disabled};

        return std::unique_ptr<mgn::Display>{nested_display_raw};
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    mi::NullInputDispatcher null_input_dispatcher;
    mir::report::null::DisplayReport null_display_report;
    mg::CloneDisplayConfigurationPolicy default_conf_policy;
    mtd::StubGLConfig stub_gl_config;
    std::shared_ptr<mtd::NullPlatform> null_platform{
        std::make_shared<mtd::NullPlatform>()};
};

}

TEST_F(NestedDisplay, respects_gl_config)
{
    using namespace testing;

    mtd::MockGLConfig mock_gl_config;
    EGLint const depth_bits{24};
    EGLint const stencil_bits{8};

    EXPECT_CALL(mock_gl_config, depth_buffer_bits())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(depth_bits));
    EXPECT_CALL(mock_gl_config, stencil_buffer_bits())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(stencil_bits));

    EXPECT_CALL(mock_egl,
                eglChooseConfig(
                    _,
                    AllOf(mtd::EGLConfigContainsAttrib(EGL_DEPTH_SIZE, depth_bits),
                          mtd::EGLConfigContainsAttrib(EGL_STENCIL_SIZE, stencil_bits)),
                    _,_,_))
        .Times(AtLeast(1))
        .WillRepeatedly(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                        SetArgPointee<4>(1),
                        Return(EGL_TRUE)));

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(mock_gl_config));
}

TEST_F(NestedDisplay, has_alpha_channel)
{
    using namespace testing;

    // mt::build_trivial_configuration sets mir_pixel_format_abgr_8888
    EXPECT_CALL(mock_egl,
                eglChooseConfig(
                    _,
                    mtd::EGLConfigContainsAttrib(EGL_ALPHA_SIZE, 8),
                    _,_,_))
        .Times(AtLeast(1))
        .WillRepeatedly(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                        SetArgPointee<4>(1),
                        Return(EGL_TRUE)));

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));
}

TEST_F(NestedDisplay, does_not_change_host_display_configuration_at_construction)
{
    using namespace testing;

    MockApplyDisplayConfigHostConnection host_connection;

    EXPECT_CALL(host_connection, apply_display_config(_))
        .Times(0);

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));
}

// Regression test for LP: #1372276
TEST_F(NestedDisplay, keeps_platform_alive)
{
    using namespace testing;

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));

    std::weak_ptr<mtd::NullPlatform> weak_platform = null_platform;
    null_platform.reset();

    EXPECT_FALSE(weak_platform.expired());
}

TEST_F(NestedDisplay, never_enables_predictive_bypass)
{   // This test can be removed after it's implemented (after nested bypass)
    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));

    int groups = 0;
    nested_display->for_each_display_sync_group(
        [&groups](mg::DisplaySyncGroup& g)
        {
            EXPECT_EQ(0, g.recommended_sleep().count());
            ++groups;
        }
    );

    ASSERT_NE(0, groups);
}

TEST_F(NestedDisplay, makes_context_current_on_creation_and_releases_on_destruction)
{
    using namespace testing;

    EXPECT_CALL(mock_egl,
                eglMakeCurrent(_, EGL_NO_SURFACE, EGL_NO_SURFACE, Ne(EGL_NO_CONTEXT)));

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));

    Mock::VerifyAndClearExpectations(&mock_egl);
    EXPECT_CALL(mock_egl,
                eglMakeCurrent(_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

TEST_F(NestedDisplay, shared_context_uses_matching_egl_attributes)
{
    using namespace testing;

    NiceMock<mtd::MockGLConfig> mock_gl_config;
    EGLint const depth_bits{24};
    EGLint const stencil_bits{8};

    std::string extensions{"EGL_KHR_surfaceless_context"};
    ON_CALL(mock_gl_config, depth_buffer_bits())
        .WillByDefault(Return(depth_bits));
    ON_CALL(mock_gl_config, stencil_buffer_bits())
        .WillByDefault(Return(stencil_bits));
    ON_CALL(mock_egl, eglChooseConfig(_,_,_,_,_))
        .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                       SetArgPointee<4>(1),
                       Return(EGL_TRUE)));
    ON_CALL(mock_egl, eglQueryString(_, EGL_EXTENSIONS))
        .WillByDefault(Return(extensions.c_str()));

    // mt::build_trivial_configuration sets mir_pixel_format_abgr_8888
    EGLint const expected_alpha_bits = 8;
    EGLint const expected_red_bits = 8;
    EGLint const expected_green_bits = 8;
    EGLint const expected_blue_bits = 8;
    EXPECT_CALL(mock_egl,
                eglChooseConfig(
                    _,
                    AllOf(mtd::EGLConfigContainsAttrib(EGL_DEPTH_SIZE, depth_bits),
                          mtd::EGLConfigContainsAttrib(EGL_STENCIL_SIZE, stencil_bits),
                          mtd::EGLConfigContainsAttrib(EGL_RED_SIZE, expected_red_bits),
                          mtd::EGLConfigContainsAttrib(EGL_GREEN_SIZE, expected_green_bits),
                          mtd::EGLConfigContainsAttrib(EGL_BLUE_SIZE, expected_blue_bits),
                          mtd::EGLConfigContainsAttrib(EGL_ALPHA_SIZE, expected_alpha_bits)),
                    _,_,_))
        .Times(AtLeast(1));

    auto const nested_display = create_nested_display(
        null_platform,
        mt::fake_shared(mock_gl_config));

    auto dummy_context = nested_display->create_gl_context();
}

TEST_F(NestedDisplay, preserves_framebuffers_for_metadata_changes)
{
    using namespace testing;

    auto display = create_nested_display(
        null_platform,
        mt::fake_shared(stub_gl_config));

    auto conf = display->configuration();

    std::vector<mg::DisplayBuffer*> initial_dbs;

    display->for_each_display_sync_group(
        [&initial_dbs](auto& sync_group)
        {
            sync_group.for_each_display_buffer(
                [&initial_dbs](auto& display_buffer)
                {
                    initial_dbs.push_back(&display_buffer);
                });
        });

    ASSERT_THAT(initial_dbs, Not(IsEmpty()));

    conf->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output)
        {
            output.scale *= 3.1415f;
            output.form_factor =
                output.form_factor == mir_form_factor_monitor ?
                    mir_form_factor_phone :
                    mir_form_factor_monitor;
            output.subpixel_arrangement =
                output.subpixel_arrangement == mir_subpixel_arrangement_horizontal_bgr ?
                    mir_subpixel_arrangement_vertical_bgr :
                    mir_subpixel_arrangement_horizontal_bgr;
            output.orientation =
                output.orientation == mir_orientation_normal ?
                    mir_orientation_inverted :
                    mir_orientation_normal;
        });

    EXPECT_THAT(*conf, Not(mt::DisplayConfigMatches(std::cref(*display->configuration()))));

    EXPECT_TRUE(display->apply_if_configuration_preserves_display_buffers(* conf));

    // Touch each of the saved DisplayBuffers, so valgrind will complain if we've invalidated them
    for (auto db : initial_dbs)
    {
        EXPECT_THAT(db->native_display_buffer(), NotNull());
    }
}

TEST_F(NestedDisplay, changing_output_does_not_preserve_framebuffers)
{
    using namespace testing;

    class MultiDisplayHostConnection : public mtd::StubHostConnection
    {
    public:
        std::shared_ptr<MirDisplayConfig> create_display_config() override
        {
            // This builds a 3-monitor configuration...
            return mt::build_non_trivial_configuration();
        }
    };


    auto display = std::make_unique<mgn::Display>(
        null_platform,
        std::make_shared<MultiDisplayHostConnection>(),
        mt::fake_shared(null_display_report),
        mt::fake_shared(default_conf_policy),
        mt::fake_shared(stub_gl_config),
        mgn::PassthroughOption::disabled);

    auto conf = display->configuration();

    // Enable the first and second displays, have the third disabled.
    conf->for_each_output(
        [counter = 0](mg::UserDisplayConfigurationOutput& output) mutable
        {
            switch (counter)
            {
                case 0:
                    output.used = true;
                    output.top_left = {0, 0};
                    break;
                case 1:
                    output.used = true;
                    output.top_left = {2000, 2000};
                    break;
                case 2:
                    output.used = false;
                    break;
            }
            ++counter;
        });

    display->configure(*conf);

    conf = display->configuration();

    // Enable the first and third displays, have the second disabled.
    conf->for_each_output(
        [counter = 0](mg::UserDisplayConfigurationOutput& output) mutable
        {
            switch (counter)
            {
                case 0:
                    output.used = true;
                    output.top_left = {0, 0};
                    break;
                case 1:
                    output.used = false;
                    break;
                case 2:
                    output.used = true;
                    output.top_left = {2000, 2000};
                    break;
            }
            ++counter;
        });

    /*
     * This tests an implementation detail; it should be perfectly possible to preserve
     * display buffers in this case, but we don't, so we shouldn't pretend to.
     */
    EXPECT_FALSE(display->apply_if_configuration_preserves_display_buffers(*conf));
}
