/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include <miral/decoration_style.h>
#include <miral/live_config_ini_file.h>

#include <mir/server.h>
#include <mir/shell/decoration/decoration_style.h>

#include <miral/test_server.h>

#include <gmock/gmock.h>

#include <sstream>

namespace msd = mir::shell::decoration;

using namespace testing;

namespace
{
msd::DecorationStyle const server_defaults;

struct TestDecorationStyle : miral::TestServer
{
    TestDecorationStyle()
    {
        start_server_in_setup = false;
    }

    void start_server_with(miral::DecorationStyle const& style)
    {
        add_server_init([this](mir::Server& server) { the_server = &server; });
        add_server_init([style](mir::Server& server) { style(server); });
        start_server();
    }

    auto applied_style() const -> std::shared_ptr<msd::DecorationStyle const>
    {
        return the_server->the_decoration_style();
    }

private:
    mir::Server* the_server = nullptr;
};
}

TEST_F(TestDecorationStyle, default_style_matches_server_defaults)
{
    start_server_with(miral::DecorationStyle{});

    EXPECT_THAT(applied_style()->titlebar_height, Eq(server_defaults.titlebar_height));
    EXPECT_THAT(applied_style()->focused.background_color, Eq(server_defaults.focused.background_color));
}

TEST_F(TestDecorationStyle, metrics_are_applied)
{
    miral::DecorationStyle style;
    style.set_metric(miral::DecorationStyle::Metric::titlebar_height, 42);
    style.set_metric(miral::DecorationStyle::Metric::side_border_width, 3);
    style.set_metric(miral::DecorationStyle::Metric::bottom_border_height, 4);

    start_server_with(style);

    EXPECT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));
    EXPECT_THAT(applied_style()->side_border_width, Eq(mir::geometry::Width{3}));
    EXPECT_THAT(applied_style()->bottom_border_height, Eq(mir::geometry::Height{4}));
}

TEST_F(TestDecorationStyle, negative_metric_values_are_ignored)
{
    miral::DecorationStyle style;
    style.set_metric(miral::DecorationStyle::Metric::titlebar_height, 42);
    style.set_metric(miral::DecorationStyle::Metric::titlebar_height, -3);

    start_server_with(style);

    EXPECT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));
}

TEST_F(TestDecorationStyle, colors_are_applied)
{
    miral::DecorationStyle style;
    style.set_color(
        miral::DecorationStyle::Element::background, miral::DecorationStyle::State::focused, 0x11, 0x22, 0x33);
    style.set_color(
        miral::DecorationStyle::Element::title, miral::DecorationStyle::State::unfocused, 0x44, 0x55, 0x66);

    start_server_with(style);

    EXPECT_THAT(applied_style()->focused.background_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
    EXPECT_THAT(applied_style()->unfocused.text_color, Eq(msd::pack_pixel(0x44, 0x55, 0x66)));
}

TEST_F(TestDecorationStyle, unfocused_button_colors_are_applied)
{
    miral::DecorationStyle style;
    style.set_color(
        miral::DecorationStyle::Element::button, miral::DecorationStyle::State::unfocused, 0x11, 0x22, 0x33);
    style.set_color(
        miral::DecorationStyle::Element::button_icon, miral::DecorationStyle::State::unfocused, 0x44, 0x55, 0x66);
    style.set_color(
        miral::DecorationStyle::Element::close_button, miral::DecorationStyle::State::unfocused, 0x77, 0x88, 0x99);
    style.set_color(
        miral::DecorationStyle::Element::close_button_icon,
        miral::DecorationStyle::State::unfocused, 0xAA, 0xBB, 0xCC);

    start_server_with(style);

    EXPECT_THAT(applied_style()->button.unfocused_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
    EXPECT_THAT(applied_style()->button.unfocused_icon_color, Eq(msd::pack_pixel(0x44, 0x55, 0x66)));
    EXPECT_THAT(applied_style()->close_button.unfocused_color, Eq(msd::pack_pixel(0x77, 0x88, 0x99)));
    EXPECT_THAT(applied_style()->close_button.unfocused_icon_color, Eq(msd::pack_pixel(0xAA, 0xBB, 0xCC)));
}

TEST_F(TestDecorationStyle, unset_button_colors_are_derived_from_the_titlebar)
{
    miral::DecorationStyle style;
    style.set_color(
        miral::DecorationStyle::Element::background, miral::DecorationStyle::State::focused, 0x00, 0x00, 0x00);
    style.set_color(
        miral::DecorationStyle::Element::title, miral::DecorationStyle::State::focused, 0x40, 0x80, 0xC0);

    start_server_with(style);

    // A quarter of the way from the background towards the title color
    EXPECT_THAT(applied_style()->button.normal_color, Eq(msd::pack_pixel(0x10, 0x20, 0x30)));
    // Halfway there while hovered
    EXPECT_THAT(applied_style()->button.active_color, Eq(msd::pack_pixel(0x20, 0x40, 0x60)));
    // Icons are drawn in the title color
    EXPECT_THAT(applied_style()->button.icon_color, Eq(msd::pack_pixel(0x40, 0x80, 0xC0)));
    EXPECT_THAT(applied_style()->close_button.icon_color, Eq(msd::pack_pixel(0x40, 0x80, 0xC0)));
    // The close button keeps its own background
    EXPECT_THAT(applied_style()->close_button.normal_color, Eq(server_defaults.close_button.normal_color));
}

TEST_F(TestDecorationStyle, explicit_button_colors_are_not_derived)
{
    miral::DecorationStyle style;
    style.set_color(
        miral::DecorationStyle::Element::background, miral::DecorationStyle::State::focused, 0x00, 0x00, 0x00);
    style.set_color(
        miral::DecorationStyle::Element::button, miral::DecorationStyle::State::normal, 0x11, 0x22, 0x33);

    start_server_with(style);

    EXPECT_THAT(applied_style()->button.normal_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
}

TEST_F(TestDecorationStyle, unrecognized_values_are_ignored){
    miral::DecorationStyle style;
    style.set_metric(static_cast<miral::DecorationStyle::Metric>(0xDEAD), 42);
    style.set_color(
        static_cast<miral::DecorationStyle::Element>(0xDEAD),
        static_cast<miral::DecorationStyle::State>(0xBEEF),
        0, 0, 0);

    start_server_with(style);

    EXPECT_THAT(applied_style()->titlebar_height, Eq(server_defaults.titlebar_height));
    EXPECT_THAT(applied_style()->focused.background_color, Eq(server_defaults.focused.background_color));
}

TEST_F(TestDecorationStyle, changes_after_start_are_applied)
{
    miral::DecorationStyle style;
    start_server_with(style);

    style.set_metric(miral::DecorationStyle::Metric::titlebar_height, 42);

    EXPECT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));
}

struct TestDecorationStyleConfig : TestDecorationStyle
{
    miral::live_config::IniFile config;
    miral::DecorationStyle style{config};

    void load(std::string const& content)
    {
        std::istringstream in{content};
        config.load_file(in, "test.config");
    }
};

TEST_F(TestDecorationStyleConfig, config_is_applied_at_startup)
{
    load("ssd_titlebar_height=42\n"
         "ssd_color_background_focused=#112233\n");

    start_server_with(style);

    EXPECT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));
    EXPECT_THAT(applied_style()->focused.background_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
}

TEST_F(TestDecorationStyleConfig, corner_radius_is_applied)
{
    load("ssd_corner_radius=12\n");

    start_server_with(style);

    EXPECT_THAT(applied_style()->corner_radius, Eq(mir::geometry::Width{12}));
}

TEST_F(TestDecorationStyleConfig, unfocused_button_colors_are_configurable)
{
    load("ssd_color_button_unfocused=#112233\n"
         "ssd_color_button_icon_unfocused=#445566\n"
         "ssd_color_close_button_unfocused=#778899\n"
         "ssd_color_close_button_icon_unfocused=#aabbcc\n");

    start_server_with(style);

    EXPECT_THAT(applied_style()->button.unfocused_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
    EXPECT_THAT(applied_style()->button.unfocused_icon_color, Eq(msd::pack_pixel(0x44, 0x55, 0x66)));
    EXPECT_THAT(applied_style()->close_button.unfocused_color, Eq(msd::pack_pixel(0x77, 0x88, 0x99)));
    EXPECT_THAT(applied_style()->close_button.unfocused_icon_color, Eq(msd::pack_pixel(0xAA, 0xBB, 0xCC)));
}

TEST_F(TestDecorationStyleConfig, unset_button_colors_follow_a_configured_titlebar_color)
{
    load("ssd_color_background_focused=#000000\n"
         "ssd_color_title_focused=#4080c0\n");

    start_server_with(style);

    EXPECT_THAT(applied_style()->button.normal_color, Eq(msd::pack_pixel(0x10, 0x20, 0x30)));
    EXPECT_THAT(applied_style()->button.icon_color, Eq(msd::pack_pixel(0x40, 0x80, 0xC0)));

    load("ssd_color_background_focused=#000000\n"
         "ssd_color_title_focused=#4080c0\n"
         "ssd_color_button_normal=#112233\n");

    EXPECT_THAT(applied_style()->button.normal_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));

    // Dropping the explicit color restores the derived one
    load("ssd_color_background_focused=#000000\n"
         "ssd_color_title_focused=#4080c0\n");

    EXPECT_THAT(applied_style()->button.normal_color, Eq(msd::pack_pixel(0x10, 0x20, 0x30)));
}

TEST_F(TestDecorationStyleConfig, config_reload_is_applied){
    start_server_with(style);

    load("ssd_side_border_width=7\n"
         "ssd_color_title_unfocused=#44556677\n");

    EXPECT_THAT(applied_style()->side_border_width, Eq(mir::geometry::Width{7}));
    EXPECT_THAT(applied_style()->unfocused.text_color, Eq(msd::pack_pixel(0x44, 0x55, 0x66, 0x77)));
}

TEST_F(TestDecorationStyleConfig, dropping_a_config_value_restores_the_default)
{
    load("ssd_titlebar_height=42\n");
    start_server_with(style);
    ASSERT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));

    load("");

    EXPECT_THAT(applied_style()->titlebar_height, Eq(server_defaults.titlebar_height));
}

TEST_F(TestDecorationStyleConfig, invalid_config_values_are_ignored)
{
    load("ssd_titlebar_height=42\n"
         "ssd_color_background_focused=#112233\n");
    start_server_with(style);

    load("ssd_titlebar_height=-1\n"
         "ssd_color_background_focused=not_a_color\n");

    EXPECT_THAT(applied_style()->titlebar_height, Eq(mir::geometry::Height{42}));
    EXPECT_THAT(applied_style()->focused.background_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
}
