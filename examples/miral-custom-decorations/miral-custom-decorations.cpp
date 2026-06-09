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

// Single-file example demonstrating miral::DecorationStrategy with visible
// custom rendering (solid green titlebar and borders, darker when buttons
// hovered). This produces obvious server-side decorations when run.
// (Limited buffer-based customization, not fully arbitrary; see header notes.)

#include <miral/decoration_strategy.h>
#include <miral/decorations.h>
#include <miral/keymap.h>
#include <miral/minimal_window_manager.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>

#include <algorithm>
#include <memory>
#include <optional>

using namespace miral;

namespace
{
class MyCustomDecoration : public DecorationStrategy
{
public:
    MyCustomDecoration() = default;
    ~MyCustomDecoration() override = default;

    // Adds titlebar height (24px) for restored/maximized windows.
    auto compute_size_with_decorations(mir::geometry::Size content_size, MirWindowType /*type*/, MirWindowState state)
        const -> mir::geometry::Size override
    {
        if (state == mir_window_state_restored || state == mir_window_state_maximized)
        {
            return {content_size.width, content_size.height + mir::geometry::Height{24}};
        }
        return content_size;
    }

    // Computes button rects inside titlebar for hover/input.
    auto button_placement(unsigned n, WindowState const& ws) const -> mir::geometry::Rectangle override
    {
        // 26x16 button rects (margin=4, spacing=4) fit in 24-high titlebar; inner glyph rect
        // tweaked (now +7/-14 +2/-4 -> 12x12 square) for non-stretched X/square/line.
        // Matches the (overridable via strategy) default 24/4/4 WindowState metrics (sides are 4 here).
        int const button_width = 26;
        int const button_height = 16;
        int const spacing = 4;
        int const margin = 4;

        int const title_w = ws.titlebar_width().as_int();
        int const title_h = ws.titlebar_height().as_int();

        int x = title_w - margin - (n + 1) * (button_width + spacing);
        int y = (title_h - button_height) / 2;

        return {{x, y}, {button_width, button_height}};
    }

    // Caches WindowState and InputState for render methods.
    void update_state(WindowState const& ws, InputState const& input) override
    {
        current_ws = ws;
        current_input = input;
    }

    auto render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override
    {
        auto const w = current_ws.titlebar_width();
        auto const h = current_ws.titlebar_height();
        if (w.as_int() <= 0 || h.as_int() <= 0)
            return std::nullopt;

        auto buffer = create_software_buffer(mir::geometry::Size{w, h});
        auto mapping = map_software_buffer(buffer);
        auto* pixels = mapping.pixels32();
        auto const stride = mapping.stride().as_uint32_t() / 4;

        uint32_t const green = pack_color(0xff, 0x00, 0xaa, 0x00);
        uint32_t const dark = pack_color(0xff, 0x00, 0x66, 0x00);

        for (int y = 0; y < h.as_int(); ++y)
            for (int x = 0; x < w.as_int(); ++x)
                pixels[y * stride + x] = green;

        for (auto const& b : current_input.buttons)
        {
            if (b.state == Button::State::hovered || b.state == Button::State::down)
            {
                auto const& r = b.rect;
                int const x0 = std::max(0, r.top_left.x.as_int());
                int const y0 = std::max(0, r.top_left.y.as_int());
                int const x1 = std::min(w.as_int(), r.top_left.x.as_int() + r.size.width.as_int());
                int const y1 = std::min(h.as_int(), r.top_left.y.as_int() + r.size.height.as_int());
                for (int y = y0; y < y1; ++y)
                    for (int x = x0; x < x1; ++x)
                        pixels[y * stride + x] = dark;
            }
        }

        // Draw button glyphs (X, square, line) in contrasting color on green titlebar.
        uint32_t const kGlyphColor = pack_color(0xff, 0x22, 0x22, 0x22);
        for (auto const& b : current_input.buttons)
        {
            auto const& r = b.rect;
            int bx = r.top_left.x.as_int() + 7;
            int by = r.top_left.y.as_int() + 2;
            int bw = r.size.width.as_int() - 14;
            int bh = r.size.height.as_int() - 4;
            if (bw <= 0 || bh <= 0)
                continue;

            if (b.function == Button::Function::close)
            {
                for (int i = 0; i < bw; ++i)
                {
                    int px1 = bx + i, py1 = by + (i * bh / bw);
                    int px2 = bx + i, py2 = by + bh - (i * bh / bw);
                    if (px1 >= 0 && px1 < w.as_int() && py1 >= 0 && py1 < h.as_int())
                        pixels[py1 * stride + px1] = kGlyphColor;
                    if (px2 >= 0 && px2 < w.as_int() && py2 >= 0 && py2 < h.as_int())
                        pixels[py2 * stride + px2] = kGlyphColor;
                }
            }
            else if (b.function == Button::Function::maximize)
            {
                for (int i = 0; i < bw; ++i)
                {
                    int px = bx + i;
                    if (px >= 0 && px < w.as_int())
                    {
                        if (by >= 0 && by < h.as_int())
                            pixels[by * stride + px] = kGlyphColor;
                        if (by + bh - 1 >= 0 && by + bh - 1 < h.as_int())
                            pixels[(by + bh - 1) * stride + px] = kGlyphColor;
                    }
                }
                for (int i = 0; i < bh; ++i)
                {
                    int py = by + i;
                    if (py >= 0 && py < h.as_int())
                    {
                        if (bx >= 0 && bx < w.as_int())
                            pixels[py * stride + bx] = kGlyphColor;
                        if (bx + bw - 1 >= 0 && bx + bw - 1 < w.as_int())
                            pixels[py * stride + bx + bw - 1] = kGlyphColor;
                    }
                }
            }
            else if (b.function == Button::Function::minimize)
            {
                int my = by + bh / 2;
                if (my >= 0 && my < h.as_int())
                    for (int i = 0; i < bw; ++i)
                    {
                        int px = bx + i;
                        if (px >= 0 && px < w.as_int())
                            pixels[my * stride + px] = kGlyphColor;
                    }
            }
        }

        auto title_pos = mir::geometry::Point{mir::geometry::X{8}, mir::geometry::Y{(h.as_int() - 14) / 2}};
        render_title_text(pixels, {w, h}, current_ws.window_name, title_pos, mir::geometry::Height{14}, kGlyphColor);

        // title text via render_title_text (FT backed)
        return buffer;
    }

    // Side borders: subtract titlebar height so they don't extend below bottom (math still
    // correct post other tweaks; uses titlebar=24 from adapter's new_window_state).
    auto render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override
    {
        auto const bw = current_ws.side_border_width;
        auto const bh =
            mir::geometry::Height{current_ws.window_size.height.as_int() - current_ws.titlebar_height().as_int()};
        if (bw.as_int() <= 0 || bh.as_int() <= 0)
            return std::nullopt;
        auto buffer = create_software_buffer(mir::geometry::Size{bw, bh});
        auto mapping = map_software_buffer(buffer);
        auto* pixels = mapping.pixels32();
        auto const stride = mapping.stride().as_uint32_t() / 4;
        uint32_t const green = pack_color(0xff, 0x00, 0xaa, 0x00);
        for (int y = 0; y < bh.as_int(); ++y)
            for (int x = 0; x < bw.as_int(); ++x)
                pixels[y * stride + x] = green;
        return buffer;
    }

    auto render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override
    {
        auto const bw = current_ws.side_border_width;
        auto const bh =
            mir::geometry::Height{current_ws.window_size.height.as_int() - current_ws.titlebar_height().as_int()};
        if (bw.as_int() <= 0 || bh.as_int() <= 0)
            return std::nullopt;
        auto buffer = create_software_buffer(mir::geometry::Size{bw, bh});
        auto mapping = map_software_buffer(buffer);
        auto* pixels = mapping.pixels32();
        auto const stride = mapping.stride().as_uint32_t() / 4;
        uint32_t const green = pack_color(0xff, 0x00, 0xaa, 0x00);
        for (int y = 0; y < bh.as_int(); ++y)
            for (int x = 0; x < bw.as_int(); ++x)
                pixels[y * stride + x] = green;
        return buffer;
    }

    // Bottom border: full width, adapter-reported height.
    auto render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override
    {
        auto const bw = current_ws.window_size.width;
        auto const bh = current_ws.bottom_border_height;
        if (bw.as_int() <= 0 || bh.as_int() <= 0)
            return std::nullopt;
        auto buffer = create_software_buffer(mir::geometry::Size{bw, bh});
        auto mapping = map_software_buffer(buffer);
        auto* pixels = mapping.pixels32();
        auto const stride = mapping.stride().as_uint32_t() / 4;
        uint32_t const green = pack_color(0xff, 0x00, 0xaa, 0x00);
        for (int y = 0; y < bh.as_int(); ++y)
            for (int x = 0; x < bw.as_int(); ++x)
                pixels[y * stride + x] = green;
        return buffer;
    }

    auto buffer_format() const -> MirPixelFormat override { return mir_pixel_format_argb_8888; }
    auto resize_corner_input_size() const -> mir::geometry::Size override
    {
        return {mir::geometry::Width{4}, mir::geometry::Height{4}};
    }

private:
    WindowState current_ws = {};
    InputState current_input = {};
};
} // namespace

int main(int argc, char const* argv[])
{
    using namespace miral;

    MirRunner runner{argc, argv};

    auto const custom_decoration = std::make_shared<MyCustomDecoration>();
    auto const decorations = CustomDecorations::strategy(custom_decoration);

    return runner.run_with({
        set_window_management_policy<MinimalWindowManager>(),
        Decorations::prefer_ssd(),
        decorations,
        WaylandExtensions{},
        Keymap{},
    });
}
