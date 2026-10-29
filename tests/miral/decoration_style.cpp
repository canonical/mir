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

#include <mir/server.h>
#include <mir/shell/decoration/decoration_style.h>

#include <miral/test_server.h>

#include <gmock/gmock.h>

namespace msd = mir::shell::decoration;

using namespace testing;

namespace
{
struct TestDecorationStyle : miral::TestServer
{
    TestDecorationStyle()
    {
        start_server_in_setup = false;
    }

    auto start_server_with(miral::DecorationStyle const& style) -> std::shared_ptr<msd::DecorationStyle const>
    {
        mir::Server* the_server = nullptr;
        std::shared_ptr<msd::DecorationStyle const> result;

        add_server_init([&](mir::Server& server) { the_server = &server; style(server); });
        add_start_callback([&] { result = the_server->the_decoration_style(); });
        start_server();

        return result;
    }
};

msd::DecorationStyle const server_defaults;
}

TEST_F(TestDecorationStyle, default_style_matches_server_defaults)
{
    auto const applied = start_server_with(miral::DecorationStyle{});

    EXPECT_THAT(applied->titlebar_height, Eq(server_defaults.titlebar_height));
    EXPECT_THAT(applied->focused.background_color, Eq(server_defaults.focused.background_color));
}

TEST_F(TestDecorationStyle, metrics_are_applied)
{
    miral::DecorationStyle style;
    style.set_metric(miral::DecorationStyle::Metric::titlebar_height, 42);
    style.set_metric(miral::DecorationStyle::Metric::side_border_width, 3);
    style.set_metric(miral::DecorationStyle::Metric::bottom_border_height, 4);

    auto const applied = start_server_with(style);

    EXPECT_THAT(applied->titlebar_height, Eq(mir::geometry::Height{42}));
    EXPECT_THAT(applied->side_border_width, Eq(mir::geometry::Width{3}));
    EXPECT_THAT(applied->bottom_border_height, Eq(mir::geometry::Height{4}));
}

TEST_F(TestDecorationStyle, colors_are_applied)
{
    miral::DecorationStyle style;
    style.set_color(
        miral::DecorationStyle::Element::background, miral::DecorationStyle::State::focused, 0x11, 0x22, 0x33);
    style.set_color(
        miral::DecorationStyle::Element::title, miral::DecorationStyle::State::unfocused, 0x44, 0x55, 0x66);

    auto const applied = start_server_with(style);

    EXPECT_THAT(applied->focused.background_color, Eq(msd::pack_pixel(0x11, 0x22, 0x33)));
    EXPECT_THAT(applied->unfocused.text_color, Eq(msd::pack_pixel(0x44, 0x55, 0x66)));
}

TEST_F(TestDecorationStyle, unrecognized_values_are_ignored)
{
    miral::DecorationStyle style;
    style.set_metric(static_cast<miral::DecorationStyle::Metric>(0xDEAD), 42);
    style.set_color(
        static_cast<miral::DecorationStyle::Element>(0xDEAD),
        static_cast<miral::DecorationStyle::State>(0xBEEF),
        0, 0, 0);

    auto const applied = start_server_with(style);

    EXPECT_THAT(applied->titlebar_height, Eq(server_defaults.titlebar_height));
    EXPECT_THAT(applied->focused.background_color, Eq(server_defaults.focused.background_color));
}
