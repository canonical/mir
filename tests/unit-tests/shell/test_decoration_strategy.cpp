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

#include "src/server/shell/decoration/decoration_strategy.h"

#include <mir/test/doubles/stub_buffer_allocator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace msd = mir::shell::decoration;
namespace mtd = mir::test::doubles;

using namespace testing;

struct DecorationStrategyStyle : Test
{
    auto strategy_with(std::shared_ptr<msd::DecorationStyle const> style) const
        -> std::shared_ptr<msd::DecorationStrategy>
    {
        return msd::DecorationStrategy::default_decoration_strategy(allocator, std::move(style));
    }

    std::shared_ptr<mtd::StubBufferAllocator> const allocator{std::make_shared<mtd::StubBufferAllocator>()};
    geom::Size const content_size{200, 100};
};

TEST_F(DecorationStrategyStyle, default_style_adds_default_borders_to_content_size)
{
    auto const strategy = strategy_with(std::make_shared<msd::DecorationStyle const>());

    EXPECT_THAT(
        strategy->compute_size_with_decorations(content_size, mir_window_type_normal, mir_window_state_restored),
        Eq(geom::Size{200 + 2 * 6, 100 + 24 + 6}));
}

TEST_F(DecorationStrategyStyle, custom_margins_are_added_to_content_size)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->titlebar_height = geom::Height{40};
    style->side_border_width = geom::Width{10};
    style->bottom_border_height = geom::Height{12};

    auto const strategy = strategy_with(style);

    EXPECT_THAT(
        strategy->compute_size_with_decorations(content_size, mir_window_type_normal, mir_window_state_restored),
        Eq(geom::Size{200 + 2 * 10, 100 + 40 + 12}));
    EXPECT_THAT(
        strategy->compute_size_with_decorations(content_size, mir_window_type_normal, mir_window_state_maximized),
        Eq(geom::Size{200, 100 + 40}));
}

TEST_F(DecorationStrategyStyle, custom_resize_corner_input_size_is_used)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->resize_corner_input_size = geom::Size{32, 48};

    EXPECT_THAT(strategy_with(style)->resize_corner_input_size(), Eq(geom::Size{32, 48}));
}

TEST_F(DecorationStrategyStyle, custom_buffer_format_is_used)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->buffer_format = mir_pixel_format_xrgb_8888;

    EXPECT_THAT(strategy_with(style)->buffer_format(), Eq(mir_pixel_format_xrgb_8888));
}
