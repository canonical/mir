/*
 * Copyright © Canonical Ltd.
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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/graphics/platform.h"
#include "src/platforms/x11/graphics/display.h"
#include "src/platforms/x11/graphics/platform.h"
#include "src/server/report/null/display_report.h"

#include "mir/graphics/display_configuration.h"

#include "mir/test/doubles/null_display_configuration_policy.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_x11.h"
#include "mir/test/doubles/mock_x11_resources.h"
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
    }

    void setup_x11_screen(
        geom::Size const& pixel,
        geom::Size const& mm,
        std::vector<mgx::X11OutputConfig> const& window_sizes)
    {
        auto const mock_xcb_conn = dynamic_cast<mtd::MockXCBConnection*>(x11_resources.conn.get());
        mock_xcb_conn->fake_screen.width_in_pixels = pixel.width.as_int();
        mock_xcb_conn->fake_screen.height_in_pixels = pixel.height.as_int();
        mock_xcb_conn->fake_screen.width_in_millimeters = mm.width.as_int();
        mock_xcb_conn->fake_screen.height_in_millimeters = mm.height.as_int();
        sizes = window_sizes;
    }
    std::shared_ptr<mgx::Display> create_display()
    {
        return std::make_shared<mgx::Display>(
                   mt::fake_shared(x11_resources),
                   nullptr,
                   "Mir on X",
                   sizes,
                   mt::fake_shared(null_display_configuration_policy),
                   std::make_shared<mir::report::null::DisplayReport>());
    }

    mtd::MockX11Resources x11_resources;
    mtd::NullDisplayConfigurationPolicy null_display_configuration_policy;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockX11> mock_x11;
    ::testing::NiceMock<mtd::MockGLConfig> mock_gl_config;
};

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
