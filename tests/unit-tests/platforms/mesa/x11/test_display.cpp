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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/platforms/mesa/server/x11/graphics/display.h"
#include "src/platforms/mesa/server/x11/graphics/platform.h"
#include "src/server/report/null/display_report.h"

#include "mir/graphics/display_configuration.h"

#include "mir/test/doubles/null_display_configuration_policy.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_x11.h"
#include "mir/test/doubles/mock_gl_config.h"
#include "mir/test/fake_shared.h"


namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace mt=mir::test;
namespace mtd=mt::doubles;
namespace geom=mir::geometry;
using namespace testing;


namespace
{


class X11DisplayTest : public ::testing::Test
{
public:
    std::vector<mgx::X11OutputConfig> sizes{{geom::Size{1280, 1024}}};

    X11DisplayTest()
    {
        using namespace testing;
        EGLint const client_version = 2;

        ON_CALL(mock_egl, eglQueryString(_, EGL_EXTENSIONS))
            .WillByDefault(Return("other stuff and EGL_CHROMIUM_sync_control"));
                    
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
            .WillByDefault(DoAll(SetArgPointee<3>(sizes[0].size.width.as_int()),
                            Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display,
                                          mock_egl.fake_egl_surface,
                                          EGL_HEIGHT,
                                          _))
            .WillByDefault(DoAll(SetArgPointee<3>(sizes[0].size.height.as_int()),
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
    }

    void setup_x11_screen(
        geom::Size const& pixel,
        geom::Size const& mm,
        std::vector<mgx::X11OutputConfig> const& window_sizes)
    {
        mock_x11.fake_x11.screen.width = pixel.width.as_int();
        mock_x11.fake_x11.screen.height = pixel.height.as_int();
        mock_x11.fake_x11.screen.mwidth = mm.width.as_int();
        mock_x11.fake_x11.screen.mheight = mm.height.as_int();
        sizes = window_sizes;

        ON_CALL(mock_x11, XGetGeometry(mock_x11.fake_x11.display,_,_,_,_,_,_,_,_))
        .WillByDefault(DoAll(SetArgPointee<5>(mock_x11.fake_x11.screen.width),
                             SetArgPointee<6>(mock_x11.fake_x11.screen.height),
                             Return(1)));
    }
    std::shared_ptr<mgx::Display> create_display()
    {
        return std::make_shared<mgx::Display>(
                   mock_x11.fake_x11.display,
                   sizes,
                   mt::fake_shared(null_display_configuration_policy),
                   mt::fake_shared(mock_gl_config),
                   std::make_shared<mir::report::null::DisplayReport>());
    }

    mtd::NullDisplayConfigurationPolicy null_display_configuration_policy;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockX11> mock_x11;
    mtd::MockGLConfig mock_gl_config;
};

}

TEST_F(X11DisplayTest, creates_display_successfully)
{
    using namespace testing;

    EXPECT_CALL(
        mock_x11,
        XCreateWindow_wrapper(
            mock_x11.fake_x11.display,
            _,
            sizes[0].size.width.as_int(),
            sizes[0].size.height.as_int()
            ,_ ,_ ,_ ,_ ,_ ,_))
        .Times(Exactly(1));

    EXPECT_CALL(mock_x11, XNextEvent(mock_x11.fake_x11.display,_))
        .Times(AtLeast(1));

    EXPECT_CALL(mock_x11, XMapWindow(mock_x11.fake_x11.display,_))
        .Times(Exactly(1));

    auto display = create_display();
}

TEST_F(X11DisplayTest, respects_gl_config)
{
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
        .Times(AtLeast(1));

    auto display = create_display();
}

TEST_F(X11DisplayTest, calculates_physical_size_of_display_based_on_default_screen)
{
    auto const pixel = geom::Size{2880, 1800};
    auto const mm = geom::Size{677, 290};
    auto const window = geom::Size{1280, 1024};
    auto const pixel_width = float(mm.width.as_int()) / float(pixel.width.as_int());
    auto const pixel_height = float(mm.height.as_int()) / float(pixel.height.as_int());
    auto const expected_size = geom::Size{window.width * pixel_width, window.height * pixel_height};

    setup_x11_screen(pixel, mm, {window});

    auto display = create_display();
    auto config = display->configuration();
    geom::Size reported_size;
    config->for_each_output(
        [&reported_size](mg::DisplayConfigurationOutput const& output)
        {
            reported_size = output.physical_size_mm;
        }
        );

    EXPECT_THAT(reported_size, Eq(expected_size));
}

TEST_F(X11DisplayTest, sets_output_scale)
{
    auto const scale = 2.5f;
    auto const pixel = geom::Size{2880, 1800};
    auto const mm = geom::Size{677, 290};
    auto const window = geom::Size{1280, 1024};

    setup_x11_screen(pixel, mm, {{window, scale}});

    auto display = create_display();
    auto config = display->configuration();
    float reported_scale = -10.0f;
    config->for_each_output(
        [&reported_scale](mg::DisplayConfigurationOutput const& output)
        {
            reported_scale = output.scale;
        }
        );

    EXPECT_THAT(reported_scale, FloatEq(scale));
}

TEST_F(X11DisplayTest, reports_a_resolution_that_matches_the_window_size)
{
    auto const pixel = geom::Size{2880, 1800};
    auto const mm = geom::Size{677, 290};
    auto const window = geom::Size{1280, 1024};

    setup_x11_screen(pixel, mm, {window});

    auto display = create_display();
    auto config = display->configuration();
    geom::Size reported_resolution;
    config->for_each_output(
        [&reported_resolution](mg::DisplayConfigurationOutput const& output)
        {
            reported_resolution = output.modes[0].size;
        }
        );

    EXPECT_THAT(reported_resolution, Eq(window));
}

TEST_F(X11DisplayTest, reports_resolutions_that_match_multiple_window_sizes)
{
    auto const pixel = geom::Size{2880, 1800};
    auto const mm = geom::Size{677, 290};
    auto const window_sizes = std::vector<mgx::X11OutputConfig>{{{1280, 1024}}, {{600, 500}}, {{20, 50}}, {{600, 500}}};

    setup_x11_screen(pixel, mm, window_sizes);

    auto display = create_display();
    auto config = display->configuration();
    auto remaining_sizes = window_sizes;
    config->for_each_output(
        [&remaining_sizes](mg::DisplayConfigurationOutput const& output)
        {
            bool output_found = false;
            for (auto i = 0u; i < remaining_sizes.size(); i++)
            {
                if (remaining_sizes[i].size == output.modes[0].size)
                {
                    output_found = true;
                    remaining_sizes.erase(remaining_sizes.begin() + i);
                    break;
                }
            }
            EXPECT_THAT(output_found, Eq(true));
        }
        );

    EXPECT_THAT(remaining_sizes.size(), Eq(0));
}

TEST_F(X11DisplayTest, multiple_outputs_are_organized_horizontally)
{
    auto const pixel = geom::Size{2880, 1800};
    auto const mm = geom::Size{677, 290};
    auto const window_sizes = std::vector<mgx::X11OutputConfig>{{{1280, 1024}}, {{600, 500}}, {{20, 50}}, {{600, 500}}};
    auto const top_lefts = std::vector<geom::Point>{{0, 0}, {1280, 0}, {1880, 0}, {1900, 0}};

    setup_x11_screen(pixel, mm, window_sizes);

    auto display = create_display();
    auto config = display->configuration();
    config->for_each_output(
        [&window_sizes, &top_lefts](mg::DisplayConfigurationOutput const& output)
        {
            bool output_found = false;
            for (auto i = 0u; i < window_sizes.size(); i++)
            {
                if (window_sizes[i].size == output.modes[0].size && top_lefts[i] == output.top_left)
                {
                    output_found = true;
                    break;
                }
            }
            EXPECT_THAT(output_found, Eq(true));
        }
        );
}

TEST_F(X11DisplayTest, adjusts_resolution_with_respect_to_screen_size)
{
    auto const pixel = geom::Size{1000, 1000};
    auto const mm = geom::Size{677, 290};
    auto const window = geom::Size{1280, 1024};
    auto const border = 150; //must match the border value in clip_to_display()

    setup_x11_screen(pixel, mm, {window});

    auto display = create_display();
    auto config = display->configuration();
    geom::Size reported_resolution;
    config->for_each_output(
        [&reported_resolution](mg::DisplayConfigurationOutput const& output)
        {
            reported_resolution = output.modes[0].size;
        });

    EXPECT_THAT(reported_resolution, Eq(geom::Size{pixel.width.as_uint32_t()-border, pixel.height.as_uint32_t()-border}));
}

TEST_F(X11DisplayTest, multiple_outputs_are_organized_horizontally_after_adjusting_resolution)
{
    auto const pixel = geom::Size{1000, 1000};
    auto const mm = geom::Size{677, 290};
    auto const border = 150; //must match the border value in clip_to_display()
    auto const window_sizes = std::vector<mgx::X11OutputConfig>{{{1280, 1024}}, {{100, 100}}};

    setup_x11_screen(pixel, mm, window_sizes);

    auto display = create_display();
    auto config = display->configuration();
    geom::Point reported_top_left;
    config->for_each_output(
        [&reported_top_left](mg::DisplayConfigurationOutput const& output)
        {
            if (output.modes[0].size == geom::Size{100, 100})
            {
                reported_top_left = output.top_left;
            }
        });

    EXPECT_THAT(reported_top_left, Eq(geom::Point{pixel.width.as_uint32_t()-border, 0}));
}

TEST_F(X11DisplayTest, updates_scale_property_correctly)
{
    auto const scale = 2.2f;
    auto display = create_display();
    auto config = display->configuration();

    config->for_each_output([&](mg::UserDisplayConfigurationOutput &conf_output)
    {
        conf_output.scale = scale;
    });
    display->configure(*config.get());

    auto new_scale = 1.0f;
    config = display->configuration();
    config->for_each_output(
        [&new_scale](mg::DisplayConfigurationOutput const& output)
        {
            new_scale = output.scale;
        }
        );

    EXPECT_THAT(new_scale, Eq(scale));
}
