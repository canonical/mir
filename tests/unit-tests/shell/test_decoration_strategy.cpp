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

#include <mir/default_font.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/test/doubles/stub_buffer_allocator.h>
#include <mir/test/doubles/stub_surface.h>

#include <bit>
#include <cmath>
#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace msd = mir::shell::decoration;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct StubWindowSurface : mtd::StubSurface
{
    StubWindowSurface(geom::Size size) : size{size} {}

    auto window_size() const -> geom::Size override { return size; }
    void resize(geom::Size const& new_size) override { size = new_size; }
    auto state() const -> MirWindowState override { return mir_window_state_restored; }
    auto name() const -> std::string override { return title; }
    auto focus_state() const -> MirWindowFocusState override { return focus; }
    void set_focus_state(MirWindowFocusState new_focus) override { focus = new_focus; }

    geom::Size size;
    std::string title{"stub"};
    MirWindowFocusState focus{mir_window_focus_state_focused};
};

/// The pixels of a rendered decoration buffer, indexed by position
struct RenderedPixels
{
    RenderedPixels(std::shared_ptr<mir::graphics::Buffer> const& buffer) :
        mappable{std::dynamic_pointer_cast<mir::renderer::software::RWMappable>(buffer)},
        mapping{mappable->map_rw()},
        size{mappable->size()},
        stride{mappable->stride()}
    {
    }

    auto operator()(int x, int y) const -> uint32_t
    {
        uint32_t pixel;
        memcpy(&pixel, mapping->data() + y * stride.as_uint32_t() + x * sizeof(pixel), sizeof(pixel));
        if constexpr (std::endian::native == std::endian::big)
            return std::byteswap(pixel);
        else
            return pixel;
    }

    std::shared_ptr<mir::renderer::software::RWMappable> const mappable;
    std::unique_ptr<mir::renderer::software::Mapping<std::byte>> const mapping;
    geom::Size const size;
    geom::Stride const stride;
};
}

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

struct DecorationStrategyRendering : DecorationStrategyStyle
{
    auto render(
        std::shared_ptr<msd::DecorationStyle const> style,
        msd::InputState const& input_state = msd::InputState{{}, {}},
        float scale = 1.0f)
        -> std::unique_ptr<msd::RendererStrategy>
    {
        strategy = strategy_with(std::move(style));
        window_state = strategy->new_window_state(surface, scale);
        auto renderer = strategy->render_strategy();
        renderer->update_state(*window_state, input_state);
        return renderer;
    }

    std::shared_ptr<StubWindowSurface> const surface{std::make_shared<StubWindowSurface>(geom::Size{200, 100})};
    std::shared_ptr<msd::DecorationStrategy> strategy;
    std::unique_ptr<msd::WindowState> window_state;
};

TEST_F(DecorationStrategyRendering, square_corners_are_opaque)
{
    auto const renderer = render(std::make_shared<msd::DecorationStyle const>());

    RenderedPixels const titlebar{renderer->render_titlebar().value()};
    EXPECT_THAT(titlebar(0, 0) >> 24, Eq(0xFFu));

    RenderedPixels const bottom{renderer->render_bottom_border().value()};
    EXPECT_THAT(bottom(0, bottom.size.height.as_int() - 1) >> 24, Eq(0xFFu));
}

TEST_F(DecorationStrategyRendering, transparent_fill_colors_are_premultiplied)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->focused.background_color = msd::pack_pixel(0xFF, 0, 0, 0);
    style->unfocused.background_color = msd::pack_pixel(0xFF, 0, 0, 0);

    auto const renderer = render(style);

    RenderedPixels const titlebar{renderer->render_titlebar().value()};
    EXPECT_THAT(titlebar(100, 0), Eq(0u));

    RenderedPixels const left{renderer->render_left_border().value()};
    EXPECT_THAT(left(0, 0), Eq(0u));
}

TEST_F(DecorationStrategyRendering, rounded_corners_are_transparent)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->corner_radius = geom::Width{12};

    auto const renderer = render(style);

    RenderedPixels const titlebar{renderer->render_titlebar().value()};
    EXPECT_THAT(titlebar(0, 0), Eq(0u));
    EXPECT_THAT(titlebar(titlebar.size.width.as_int() - 1, 0), Eq(0u));
    // Away from the corners the titlebar is untouched
    EXPECT_THAT(titlebar(titlebar.size.width.as_int() / 2, 0) >> 24, Eq(0xFFu));
    EXPECT_THAT(titlebar(0, titlebar.size.height.as_int() - 1) >> 24, Eq(0xFFu));

    RenderedPixels const bottom{renderer->render_bottom_border().value()};
    auto const bottom_row = bottom.size.height.as_int() - 1;
    EXPECT_THAT(bottom(0, bottom_row), Eq(0u));
    EXPECT_THAT(bottom(bottom.size.width.as_int() - 1, bottom_row), Eq(0u));
    EXPECT_THAT(bottom(bottom.size.width.as_int() / 2, bottom_row) >> 24, Eq(0xFFu));
}

TEST_F(DecorationStrategyRendering, corner_radius_larger_than_the_bottom_border_does_not_round_the_side_borders)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->corner_radius = geom::Width{20};

    auto const renderer = render(style);

    RenderedPixels const left{renderer->render_left_border().value()};
    EXPECT_THAT(left(0, left.size.height.as_int() - 1), Ne(0u));
    EXPECT_THAT(left(0, 0) >> 24, Eq(0xFFu));
}

TEST_F(DecorationStrategyRendering, buttons_are_drawn_with_the_unfocused_colors_while_unfocused)
{
    auto style = std::make_shared<msd::DecorationStyle>();
    style->button.normal_color = msd::pack_pixel(0x10, 0x20, 0x30);
    style->button.unfocused_color = msd::pack_pixel(0x40, 0x50, 0x60);

    geom::Rectangle const button_rect{{100, 0}, {24, 24}};
    msd::InputState const input_state{{msd::Button{msd::Button::Maximize, msd::Button::Up, button_rect}}, {}};

    auto const focused_renderer = render(style, input_state);
    RenderedPixels const focused_titlebar{focused_renderer->render_titlebar().value()};
    EXPECT_THAT(focused_titlebar(101, 1), Eq(0xFF102030u));

    surface->set_focus_state(mir_window_focus_state_unfocused);

    auto const unfocused_renderer = render(style, input_state);
    RenderedPixels const unfocused_titlebar{unfocused_renderer->render_titlebar().value()};
    EXPECT_THAT(unfocused_titlebar(101, 1), Eq(0xFF405060u));
}

struct DecorationTitleRendering : DecorationStrategyRendering, WithParamInterface<float>
{
    void SetUp() override
    {
        if (mir::default_font().empty())
            GTEST_SKIP() << "No decoration font available";
    }

    auto title_pixels(std::string const& title, msd::InputState const& input_state, int left, int right)
        -> std::vector<uint32_t>
    {
        surface->title = title;
        auto const renderer = render(std::make_shared<msd::DecorationStyle const>(), input_state, GetParam());
        RenderedPixels const pixels{renderer->render_titlebar().value()};
        std::vector<uint32_t> result;
        for (int y = 0; y < pixels.size.height.as_int(); ++y)
            for (int x = left; x < right; ++x)
                result.push_back(pixels(x, y));
        return result;
    }

    msd::InputState const buttons{{
        msd::Button{msd::Button::Close, msd::Button::Up, {{160, 0}, {24, 24}}},
        msd::Button{msd::Button::Minimize, msd::Button::Up, {{100, 0}, {24, 24}}}}, {}};
};

TEST_P(DecorationTitleRendering, long_title_does_not_draw_under_or_between_buttons)
{
    auto const left = lround(100 * GetParam());
    auto const right = lround(200 * GetParam());

    EXPECT_THAT(
        title_pixels(std::string(100, 'W'), buttons, left, right),
        Eq(title_pixels("", buttons, left, right)));
}

TEST_P(DecorationTitleRendering, short_title_is_unchanged_when_buttons_are_present)
{
    auto const right = lround(100 * GetParam());
    auto const expected = title_pixels("Title", {{}, {}}, 0, right);

    EXPECT_THAT(title_pixels("Title", buttons, 0, right), Eq(expected));
    EXPECT_THAT(expected, Ne(title_pixels("", buttons, 0, right)));
}

TEST_P(DecorationTitleRendering, overlapping_glyph_extents_do_not_cause_unnecessary_ellipsis)
{
    auto const width = lround(200 * GetParam());
    auto const full_title = title_pixels("ffffffffff", {{}, {}}, 0, width);
    auto const background = title_pixels("", {{}, {}}, 0, width);
    int last_pixel{0};
    for (size_t i = 0; i < full_title.size(); ++i)
        if (full_title[i] != background[i])
            last_pixel = std::max(last_pixel, static_cast<int>(i % width));

    int const button_left = std::ceil((last_pixel + 2) / GetParam());
    msd::InputState const tight_buttons{{
        msd::Button{msd::Button::Close, msd::Button::Up, {{button_left, 0}, {24, 24}}}}, {}};
    auto const right = lround(button_left * GetParam());
    EXPECT_THAT(
        title_pixels("ffffffffff", tight_buttons, 0, right),
        Eq(title_pixels("ffffffffff", {{}, {}}, 0, right)));
}

TEST_P(DecorationTitleRendering, long_titles_preserve_both_ends_with_a_middle_ellipsis)
{
    for (auto const& [first, last] : std::vector<std::pair<std::string, std::string>>{
        {"A", "Z"}, {"é", "Ω"}, {"e\u0301", "Ω\u0301"}})
    {
        SCOPED_TRACE(first + "…" + last);
        surface->resize({200, 100});
        std::string title;
        for (int i = 0; i < 50; ++i)
            title += first;
        for (int i = 0; i < 50; ++i)
            title += last;
        auto const right = lround(100 * GetParam());
        auto const actual = title_pixels(title, buttons, 0, right);
        EXPECT_THAT(actual, Ne(title_pixels("", buttons, 0, right)));

        // Render candidate abbreviations on a wide titlebar so the expected text is not itself ellipsized.
        surface->resize({1000, 100});
        bool matched{false};
        std::string prefix;
        for (int i = 1; i < 16 && !matched; ++i)
        {
            prefix += first;
            std::string suffix;
            for (int j = 1; j < 16 && !matched; ++j)
            {
                suffix += last;
                matched = actual == title_pixels(prefix + "…" + suffix, {{}, {}}, 0, right);
            }
        }
        EXPECT_TRUE(matched);
    }
}

TEST_P(DecorationTitleRendering, title_is_omitted_when_even_the_ellipsis_does_not_fit)
{
    auto const right = lround(200 * GetParam());
    for (auto const button_left : {0, 8, 9})
    {
        SCOPED_TRACE(button_left);
        msd::InputState const cramped_buttons{{
            msd::Button{msd::Button::Close, msd::Button::Up, {{button_left, 0}, {24, 24}}}}, {}};
        EXPECT_THAT(
            title_pixels("Long window title", cramped_buttons, 0, right),
            Eq(title_pixels("", cramped_buttons, 0, right)));
    }
}

INSTANTIATE_TEST_SUITE_P(Scales, DecorationTitleRendering, Values(1.0f, 1.5f, 2.0f));
