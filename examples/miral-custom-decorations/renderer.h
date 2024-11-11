/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_CUSTOM_DECORATIONS_RENDERER_H
#define MIRAL_CUSTOM_DECORATIONS_RENDERER_H

#include "theme.h"
#include "window.h"

#include "mir_toolkit/common.h"

#include <memory>
#include <optional>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
}
namespace shell
{
}
}

namespace
{
auto const buffer_format = mir_pixel_format_argb_8888;
auto const bytes_per_pixel = 4;
}

class Renderer
{
public:
    Renderer(
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        Theme focused,
        Theme unfocused,
        ButtonTheme button_theme);

    void update_state(WindowState const& window_state, InputState const& input_state);
    auto render_titlebar() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;
    auto render_left_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;
    auto render_right_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;
    auto render_bottom_border() -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;

private:
    class Text
    {
    public:
        static auto instance() -> std::shared_ptr<Text>;

        virtual ~Text() = default;

        virtual void render(
            Pixel* buf,
            mir::geometry::Size buf_size,
            std::string const& text,
            mir::geometry::Point top_left,
            mir::geometry::Height height_pixels,
            Pixel color) = 0;

    private:
        class Impl;
        class Null;

        static std::mutex static_mutex;
        static std::weak_ptr<Text> singleton;
    };

    std::shared_ptr<mir::graphics::GraphicBufferAllocator> buffer_allocator;
    ::Theme const focused_theme;
    ::Theme const unfocused_theme;
    ::Theme const* current_theme;
    std::map<ButtonFunction, std::pair<Icon const, Icon::DrawingFunction const>> button_icons;
    std::shared_ptr<StaticGeometry const> const static_geometry;

    bool needs_solid_color_redraw{true};
    mir::geometry::Size left_border_size;
    mir::geometry::Size right_border_size;
    mir::geometry::Size bottom_border_size;
    size_t solid_color_pixels_length{0};
    size_t scaled_solid_color_pixels_length{0};
    std::unique_ptr<Pixel[]> solid_color_pixels; // can be nullptr

    mir::geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    bool needs_titlebar_redraw{true};
    bool needs_titlebar_buttons_redraw{true};
    std::string name;
    std::vector<ButtonInfo> buttons;

    std::shared_ptr<Text> const text;

    float scale{1.0f};

    void update_solid_color_pixels();
    auto make_buffer(
        Pixel const* pixels,
        mir::geometry::Size size) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>;
    static auto alloc_pixels(mir::geometry::Size size) -> std::unique_ptr<Pixel[]>;
};

#endif // MIR_SHELL_DECORATION_RENDERER_H_
