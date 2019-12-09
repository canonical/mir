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


#include "renderer.h"
#include "window.h"
#include "input.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/log.h"

#include "boost/throw_exception.hpp"

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

namespace
{
inline auto area(geom::Size size) -> size_t
{
    return (size.width > geom::Width{} && size.height > geom::Height{})
        ? size.width.as_int() * size.height.as_int()
        : 0;

}

inline void render_row(
    uint32_t* const data,
    geom::Size buf_size,
    geom::Point left,
    geom::Width length,
    uint32_t color)
{
    if (left.y < geom::Y{} || left.y >= as_y(buf_size.height))
        return;
    geom::X const right = std::min(left.x + as_delta(length), as_x(buf_size.width));
    left.x = std::max(left.x, geom::X{});
    uint32_t* const start = data + (left.y.as_int() * buf_size.width.as_int()) + left.x.as_int();
    uint32_t* const end = start + right.as_int() - left.x.as_int();
    for (uint32_t* i = start; i < end; i++)
        *i = color;
}
}

msd::Renderer::Renderer(std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator)
    : buffer_allocator{buffer_allocator},
      theme{
          /*   focuesed */ color(0x32, 0x32, 0x32, 0xFF),
          /*  unfocused */ color(0x54, 0x54, 0x54, 0xFF)}
{
}

auto msd::Renderer::render_titlebar(
    WindowState const& window_state,
    InputState const& input_state) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    bool full_render{false};

    if (!area(window_state.titlebar_rect().size))
        return std::experimental::nullopt;

    if (window_state.titlebar_rect().size != titlebar_size)
    {
        titlebar_size = window_state.titlebar_rect().size;
        titlebar_pixels = alloc_pixels(titlebar_size);
        full_render = true;
    }

    if (background_color(window_state) != cached_titlebar_color)
    {
        cached_titlebar_color = background_color(window_state);
        full_render = true;
    }

    if (window_state.window_name() != cached_name)
    {
        cached_name = window_state.window_name();
        full_render = true;
    }

    if (full_render)
    {
        for (geom::Y y{0}; y < as_y(titlebar_size.height); y += geom::DeltaY{1})
        {
            render_row(
                titlebar_pixels.get(), titlebar_size,
                {0, y}, titlebar_size.width,
                cached_titlebar_color);
        }
    }

    if (full_render || input_state.buttons() != cached_buttons)
    {
        cached_buttons = input_state.buttons();
        for (auto const& button : cached_buttons)
        {
            Pixel button_color = color(0x80, 0x80, 0x80, 0xFF);
            if (button.state == ButtonState::Hovered)
                button_color = color(0xA0, 0xA0, 0xA0, 0xFF);
            for (geom::Y y{button.rect.top()}; y < button.rect.bottom(); y += geom::DeltaY{1})
            {
                render_row(
                    titlebar_pixels.get(), titlebar_size,
                    {button.rect.left(), y}, button.rect.size.width,
                    button_color);
            }
        }
    }

    return make_buffer(titlebar_pixels.get(), titlebar_size);
}

auto msd::Renderer::render_left_border(
    WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(window_state.left_border_rect().size))
        return std::experimental::nullopt;
    update_solid_color_pixels(window_state);
    return make_buffer(solid_color_pixels.get(), window_state.left_border_rect().size);
}

auto msd::Renderer::render_right_border(
    WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(window_state.right_border_rect().size))
        return std::experimental::nullopt;
    update_solid_color_pixels(window_state);
    return make_buffer(solid_color_pixels.get(), window_state.right_border_rect().size);
}

auto msd::Renderer::render_bottom_border(
    WindowState const& window_state) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(window_state.bottom_border_rect().size))
        return std::experimental::nullopt;
    update_solid_color_pixels(window_state);
    return make_buffer(solid_color_pixels.get(), window_state.bottom_border_rect().size);
}

void msd::Renderer::update_solid_color_pixels(WindowState const& window_state)
{
    bool needs_redraw{false};

    size_t length{};
    length = std::max(length, area(window_state.left_border_rect().size));
    length = std::max(length, area(window_state.right_border_rect().size));
    length = std::max(length, area(window_state.bottom_border_rect().size));

    if (length != solid_color_pixels_length)
    {
        solid_color_pixels_length = length;
        solid_color_pixels = alloc_pixels(geom::Size{length, 1});
        needs_redraw = true;
    }

    if (background_color(window_state) != solid_color_pixels_color)
    {
        solid_color_pixels_color = background_color(window_state);
        needs_redraw = true;
    }

    if (needs_redraw)
    {
        render_row(
            solid_color_pixels.get(), geom::Size{solid_color_pixels_length, 1},
            geom::Point{}, geom::Width{solid_color_pixels_length},
            solid_color_pixels_color);
    }
}

auto msd::Renderer::background_color(WindowState const& window_state) const -> Pixel
{
    return window_state.focused_state() == mir_window_focus_state_focused ? theme.focused : theme.unfocused;
}

auto msd::Renderer::make_buffer(
    uint32_t const* pixels,
    geometry::Size size) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(size))
    {
        log_warning("Failed to draw SSD: tried to create zero size buffer");
        return std::experimental::nullopt;
    }
    std::shared_ptr<graphics::Buffer> const buffer = buffer_allocator->alloc_software_buffer(size, buffer_format);
    auto const pixel_source = dynamic_cast<mrs::PixelSource*>(buffer->native_buffer_base());
    if (!pixel_source)
    {
        log_warning("Failed to draw SSD: software buffer not a pixel source");
        return std::experimental::nullopt;
    }
    size_t const buf_pixels_size{area(size) * sizeof(Pixel)};
    pixel_source->write((unsigned char const*)pixels, buf_pixels_size);
    return buffer;
}

auto msd::Renderer::alloc_pixels(geometry::Size size) -> std::unique_ptr<uint32_t[]>
{
    size_t const buf_size = area(size) * bytes_per_pixel;
    if (buf_size)
        return std::unique_ptr<uint32_t[]>{new uint32_t[buf_size]};
    else
        return nullptr;
}

auto msd::Renderer::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) -> uint32_t
{
    return ((uint32_t)b <<  0) |
           ((uint32_t)g <<  8) |
           ((uint32_t)r << 16) |
           ((uint32_t)a << 24);
}
