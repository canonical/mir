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


#include "renderer.h"
#include "window.h"
#include "input.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/log.h"

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
}

msd::Renderer::Renderer(
std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
    std::unique_ptr<RendererStrategy>  strategy)
    : buffer_allocator{buffer_allocator},
    strategy{std::move(strategy)}
{
}

auto msd::Renderer::make_buffer(
    uint32_t const* pixels,
    geometry::Size size,
    MirPixelFormat buffer_format) -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(size))
    {
        log_warning("Failed to draw SSD: tried to create zero size buffer");
        return std::nullopt;
    }

    if (sizeof(Pixel) != MIR_BYTES_PER_PIXEL(buffer_format))
    {
        log_warning("Failed to draw SSD: tried to create buffer with unsupported format: %d", buffer_format);
        return std::nullopt;
    }

    try
    {
        return mrs::alloc_buffer_with_content(
            *buffer_allocator,
            reinterpret_cast<unsigned char const*>(pixels),
            size,
            geom::Stride{size.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(buffer_format)},
            buffer_format);
    }
    catch (std::runtime_error const&)
    {
        log_warning("Failed to draw SSD: software buffer not a pixel source");
        return std::nullopt;
    }
}

auto msd::Renderer::render_titlebar() -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (auto const& rendered_pixels = strategy->render_titlebar())
    {
        return make_buffer(rendered_pixels->pixels, rendered_pixels->size, rendered_pixels->format);
    }
    else
    {
        return std::nullopt;
    }
}

auto msd::Renderer::render_left_border() -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (auto const& rendered_pixels = strategy->render_left_border())
    {
        return make_buffer(rendered_pixels->pixels, rendered_pixels->size, rendered_pixels->format);
    }
    else
    {
        return std::nullopt;
    }
}

auto msd::Renderer::render_right_border() -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (auto const& rendered_pixels = strategy->render_right_border())
    {
        return make_buffer(rendered_pixels->pixels, rendered_pixels->size, rendered_pixels->format);
    }
    else
    {
        return std::nullopt;
    }
}

auto msd::Renderer::render_bottom_border() -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (auto const& rendered_pixels = strategy->render_bottom_border())
    {
        return make_buffer(rendered_pixels->pixels, rendered_pixels->size, rendered_pixels->format);
    }
    else
    {
        return std::nullopt;
    }
}


void msd::Renderer::update_state(WindowState const& window_state, InputState const& input_state)
{
    strategy->update_state(window_state, input_state);
}
