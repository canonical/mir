/*
 * Copyright © 2014 Canonical Ltd.
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
    std::shared_ptr<MirDisplayConfiguration> create_display_config() override
    {
        return mt::build_trivial_configuration();
    }
};

class MockApplyDisplayConfigHostConnection : public SingleDisplayHostConnection
{
public:
    MOCK_METHOD1(apply_display_config, void(MirDisplayConfiguration&));
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
            mt::fake_shared(null_input_dispatcher),
            mt::fake_shared(null_display_report),
            mt::fake_shared(default_conf_policy),
            gl_config, std::make_shared<mtd::StubCursorListener>()};

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
