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

// Demonstrates limited buffer-based custom SSD via miral::DecorationStrategy.
// Visibly different from the built-in theme: green chrome, right-aligned buttons
// (matching the default Mir layout), title text on the left.
// See doc/sphinx/how-to/custom-server-side-decorations.md for placement notes.

#include <miral/custom_decorations.h>
#include <miral/decorations.h>
#include <miral/keymap.h>
#include <miral/minimal_window_manager.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

namespace geom = mir::geometry;

namespace
{
using miral::pack_color;

auto scaled_pixels(int logical, float scale) -> int
{
    return std::max(1, static_cast<int>(std::lround(logical * scale)));
}

auto scaled_size(geom::Size logical, float scale) -> geom::Size
{
    return {
        geom::Width{scaled_pixels(logical.width.as_int(), scale)},
        geom::Height{scaled_pixels(logical.height.as_int(), scale)}};
}

class MyCustomDecoration : public miral::DecorationStrategy
{
public:
    auto compute_size_with_decorations(geom::Size content_size, MirWindowType /*type*/, MirWindowState state) const
        -> geom::Size override
    {
        if (state == mir_window_state_restored)
        {
            auto const side = side_border_width().as_int();
            auto const bottom = bottom_border_height().as_int();
            auto const title = titlebar_height().as_int();
            return {
                geom::Width{content_size.width.as_int() + 2 * side},
                geom::Height{content_size.height.as_int() + title + bottom}};
        }
        if (state == mir_window_state_maximized)
        {
            return {content_size.width, content_size.height + titlebar_height()};
        }
        return content_size;
    }

    auto button_placement(unsigned n, WindowState const& ws) const -> geom::Rectangle override
    {
        int const button_width = 26;
        int const button_height = 16;
        int const spacing = 4;
        int const margin = 4;
        int const side = ws.side_border_width().as_int();
        int const titlebar_w = ws.titlebar_width().as_int();

        // n=0 is the rightmost button (close), matching the built-in default strategy.
        int const x = titlebar_w - side - margin - (n + 1) * button_width - n * spacing;
        int const y = (ws.titlebar_height().as_int() - button_height) / 2;

        return {{x, y}, {button_width, button_height}};
    }

    auto render_titlebar(WindowState const& ws, InputState const& input, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface> override
    {
        auto const logical = geom::Size{ws.titlebar_width(), ws.titlebar_height()};
        auto const size = scaled_size(logical, ws.scale());
        if (size.width.as_int() <= 0 || size.height.as_int() <= 0)
            return std::nullopt;

        auto pixels = buffers.create_mapping(size, buffer_format());
        auto* px = pixels.mapping.pixels32();
        auto const stride = pixels.mapping.stride().as_uint32_t() / 4;
        auto const w = size.width.as_int();
        auto const h = size.height.as_int();

        uint32_t const green = pack_color(0xcc, 0x00, 0xaa, 0x00);
        uint32_t const dark = pack_color(0xee, 0x00, 0x66, 0x00);

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                px[y * stride + x] = green;

        for (auto const& b : input.buttons())
        {
            if (b.state == Button::State::hovered || b.state == Button::State::down)
            {
                auto const& r = b.rect;
                int const scale = scaled_pixels(1, ws.scale());
                int const x0 = std::max(0, r.top_left.x.as_int() * scale);
                int const y0 = std::max(0, r.top_left.y.as_int() * scale);
                int const x1 = std::min(w, (r.top_left.x.as_int() + r.size.width.as_int()) * scale);
                int const y1 = std::min(h, (r.top_left.y.as_int() + r.size.height.as_int()) * scale);
                for (int y = y0; y < y1; ++y)
                    for (int x = x0; x < x1; ++x)
                        px[y * stride + x] = dark;
            }
        }

        draw_glyphs(px, stride, w, h, ws.scale(), input);
        auto const text_h = scaled_pixels(14, ws.scale());
        // Left-aligned title, matching the built-in default (8, 2) logical px origin.
        auto title_pos = geom::Point{
            geom::X{scaled_pixels(8, ws.scale())},
            geom::Y{scaled_pixels(2, ws.scale())}};
        render_title_text(
            px, size, ws.window_name(), title_pos, geom::Height{text_h}, pack_color(0xff, 0x22, 0x22, 0x22));

        return std::move(pixels.surface);
    }

    auto render_left_border(WindowState const& ws, InputState const& /*input*/, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface> override
    {
        return render_side_border(ws, buffers);
    }

    auto render_right_border(WindowState const& ws, InputState const& /*input*/, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface> override
    {
        return render_side_border(ws, buffers);
    }

    auto render_bottom_border(WindowState const& ws, InputState const& /*input*/, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface> override
    {
        auto const logical = geom::Size{ws.window_size().width, ws.bottom_border_height()};
        auto const size = scaled_size(logical, ws.scale());
        if (size.width.as_int() <= 0 || size.height.as_int() <= 0)
            return std::nullopt;
        return fill_green(buffers, size);
    }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }

    auto resize_corner_input_size() const -> geom::Size override { return {geom::Width{4}, geom::Height{4}}; }

private:
    static auto fill_green(miral::DecorationBuffers& buffers, geom::Size size)
        -> std::optional<miral::DecorationSurface>
    {
        auto pixels = buffers.create_mapping(size, mir_pixel_format_argb_8888);
        auto* px = pixels.mapping.pixels32();
        auto const stride = pixels.mapping.stride().as_uint32_t() / 4;
        uint32_t const green = pack_color(0xff, 0x00, 0xaa, 0x00);
        for (int y = 0; y < size.height.as_int(); ++y)
            for (int x = 0; x < size.width.as_int(); ++x)
                px[y * stride + x] = green;
        return std::move(pixels.surface);
    }

    auto render_side_border(WindowState const& ws, miral::DecorationBuffers& buffers)
        -> std::optional<miral::DecorationSurface>
    {
        auto const logical = geom::Size{
            ws.side_border_width(), geom::Height{ws.window_size().height.as_int() - ws.titlebar_height().as_int()}};
        auto const size = scaled_size(logical, ws.scale());
        if (size.width.as_int() <= 0 || size.height.as_int() <= 0)
            return std::nullopt;
        return fill_green(buffers, size);
    }

    static void draw_glyphs(uint32_t* pixels, int stride, int w, int h, float scale, InputState const& input)
    {
        uint32_t const kGlyphColor = pack_color(0xff, 0x22, 0x22, 0x22);
        for (auto const& b : input.buttons())
        {
            auto const& r = b.rect;
            int const bx = scaled_pixels(r.top_left.x.as_int() + 7, scale);
            int const by = scaled_pixels(r.top_left.y.as_int() + 2, scale);
            int const bw = scaled_pixels(r.size.width.as_int() - 14, scale);
            int const bh = scaled_pixels(r.size.height.as_int() - 4, scale);
            if (bw <= 0 || bh <= 0)
                continue;

            if (b.function == Button::Function::close)
            {
                for (int i = 0; i < bw; ++i)
                {
                    int px1 = bx + i, py1 = by + (i * bh / bw);
                    int px2 = bx + i, py2 = by + bh - (i * bh / bw);
                    if (px1 >= 0 && px1 < w && py1 >= 0 && py1 < h)
                        pixels[py1 * stride + px1] = kGlyphColor;
                    if (px2 >= 0 && px2 < w && py2 >= 0 && py2 < h)
                        pixels[py2 * stride + px2] = kGlyphColor;
                }
            }
            else if (b.function == Button::Function::maximize)
            {
                for (int i = 0; i < bw; ++i)
                {
                    int px = bx + i;
                    if (px >= 0 && px < w)
                    {
                        if (by >= 0 && by < h)
                            pixels[by * stride + px] = kGlyphColor;
                        if (by + bh - 1 >= 0 && by + bh - 1 < h)
                            pixels[(by + bh - 1) * stride + px] = kGlyphColor;
                    }
                }
                for (int i = 0; i < bh; ++i)
                {
                    int py = by + i;
                    if (py >= 0 && py < h)
                    {
                        if (bx >= 0 && bx < w)
                            pixels[py * stride + bx] = kGlyphColor;
                        if (bx + bw - 1 >= 0 && bx + bw - 1 < w)
                            pixels[py * stride + bx + bw - 1] = kGlyphColor;
                    }
                }
            }
            else if (b.function == Button::Function::minimize)
            {
                int my = by + bh / 2;
                if (my >= 0 && my < h)
                    for (int i = 0; i < bw; ++i)
                    {
                        int px = bx + i;
                        if (px >= 0 && px < w)
                            pixels[my * stride + px] = kGlyphColor;
                    }
            }
        }
    }
};
} // namespace

int main(int argc, char const* argv[])
{
    miral::MirRunner runner{argc, argv};

    return runner.run_with({
        miral::set_window_management_policy<miral::MinimalWindowManager>(),
        miral::Decorations::prefer_ssd(),
        miral::CustomDecorations{std::make_shared<MyCustomDecoration>()},
        miral::WaylandExtensions{},
        miral::Keymap{},
    });
}
