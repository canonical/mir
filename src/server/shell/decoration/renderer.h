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

#ifndef MIR_SHELL_DECORATION_RENDERER_H_
#define MIR_SHELL_DECORATION_RENDERER_H_

#include "mir/geometry/rectangle.h"

#include "input.h"

#include <memory>
#include <map>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
}
namespace shell
{
namespace decoration
{
class WindowState;
class InputState;
struct StaticGeometry;

auto const buffer_format = mir_pixel_format_argb_8888;

class Renderer
{
public:
    Renderer(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator);

    virtual ~Renderer() = default;

    virtual void update_state(WindowState const& window_state, InputState const& input_state) = 0;
    virtual auto render_titlebar() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_left_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_right_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;
    virtual auto render_bottom_border() -> std::optional<std::shared_ptr<graphics::Buffer>> = 0;

    using Pixel = uint32_t;
    static auto alloc_pixels(geometry::Size size) -> std::unique_ptr<Pixel[]>;

    auto make_buffer(
        Pixel const* pixels,
        geometry::Size size,
        MirPixelFormat buffer_format) -> std::optional<std::shared_ptr<graphics::Buffer>>;

private:
    std::shared_ptr<graphics::GraphicBufferAllocator> buffer_allocator;
};

class RendererImpl : public Renderer
{
public:
    RendererImpl(
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<StaticGeometry const> const& static_geometry);

private:
    std::shared_ptr<StaticGeometry const> const static_geometry;

    /// A visual theme for a decoration
    /// Focused and unfocused windows use a different theme
    struct Theme
    {
        Pixel const background_color;   ///< Color for background of the titlebar and borders
        Pixel const text_color;         ///< Color the window title is drawn in
    };
    Theme const focused_theme;
    Theme const unfocused_theme;
    Theme const* current_theme;

    class Text
    {
    public:
        static auto instance() -> std::shared_ptr<Text>;

        virtual ~Text() = default;

        virtual void render(
            Pixel* buf,
            geometry::Size buf_size,
            std::string const& text,
            geometry::Point top_left,
            geometry::Height height_pixels,
            Pixel color) = 0;

    private:
        class Impl;
        class Null;

        static std::mutex static_mutex;
        static std::weak_ptr<Text> singleton;
    };
    std::shared_ptr<Text> const text;

    // Info needed to render a button icon
    struct Icon
    {
        Pixel const normal_color;   ///< Normal of the background of the button area
        Pixel const active_color;   ///< Color of the background when button is active
        Pixel const icon_color;     ///< Color of the icon
        std::function<void(
            Pixel* const data,
            geometry::Size buf_size,
            geometry::Rectangle box,
            geometry::Width line_width,
            Pixel color)> const render_icon; ///< Draws button's icon to the given buffer
    };
    std::map<ButtonFunction, Icon const> button_icons;

    float scale{1.0f};
    std::string name;
    geometry::Size left_border_size;
    geometry::Size right_border_size;
    geometry::Size bottom_border_size;

    bool needs_solid_color_redraw{true};

    size_t solid_color_pixels_length{0};
    size_t scaled_solid_color_pixels_length{0};
    std::unique_ptr<Pixel[]> solid_color_pixels; // can be nullptr

    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    bool needs_titlebar_redraw{true};
    bool needs_titlebar_buttons_redraw{true};

    std::vector<ButtonInfo> buttons;

    auto render_titlebar() -> std::optional<std::shared_ptr<graphics::Buffer>> override;
    auto render_left_border() -> std::optional<std::shared_ptr<graphics::Buffer>> override;
    auto render_right_border() -> std::optional<std::shared_ptr<graphics::Buffer>> override;
    auto render_bottom_border() -> std::optional<std::shared_ptr<graphics::Buffer>> override;
    void update_state(WindowState const& window_state, InputState const& input_state) override;

    void update_solid_color_pixels();

    void set_focus_state(MirWindowFocusState focus_state);

    void redraw_titlebar_background(geometry::Size scaled_titlebar_size);
    void redraw_titlebar_text(geometry::Size scaled_titlebar_size);
    void redraw_titlebar_buttons(geometry::Size scaled_titlebar_size);
};
}
}
}

#endif // MIR_SHELL_DECORATION_RENDERER_H_
