/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SHELL_DECORATION_RENDERER_H_
#define MIR_SHELL_DECORATION_RENDERER_H_

#include "mir/geometry/rectangle.h"

#include "input.h"

#include <memory>

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

auto const buffer_format = mir_pixel_format_argb_8888;
auto const bytes_per_pixel = 4;

class Renderer
{
public:
    Renderer(std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator);

    auto render_titlebar(
        WindowState const& window_state,
        InputState const& input_state) -> std::experimental::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_left_border(
        WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_right_border(
        WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<graphics::Buffer>>;
    auto render_bottom_border(
        WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<graphics::Buffer>>;

private:
    using Pixel = uint32_t;

    struct Theme
    {
        Pixel focused;
        Pixel unfocused;
    };

    std::shared_ptr<graphics::GraphicBufferAllocator> buffer_allocator;
    Theme theme;

    size_t solid_color_pixels_length{0};
    Pixel solid_color_pixels_color{0};
    std::unique_ptr<Pixel[]> solid_color_pixels; // can be nullptr

    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    std::string cached_name;
    Pixel cached_titlebar_color;
    std::vector<ButtonInfo> cached_buttons;

    void update_solid_color_pixels(WindowState const& window_state);
    auto background_color(WindowState const& window_state) const -> Pixel;
    auto make_buffer(
        Pixel const* pixels,
        geometry::Size size) -> std::experimental::optional<std::shared_ptr<graphics::Buffer>>;
    static auto alloc_pixels(geometry::Size size) -> std::unique_ptr<Pixel[]>;
    static auto color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) -> Pixel;
};
}
}
}

#endif // MIR_SHELL_DECORATION_RENDERER_H_
