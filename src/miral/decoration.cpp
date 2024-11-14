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

#include "mir/geometry/forward.h"
#include "mir/shell/decoration/input_resolver.h"
#include "miral/decoration.h"

#include "mir/server.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/wayland/weak.h"
#include "mir/shell/decoration.h"
#include "miral/decoration_manager_builder.h"
#include "miral/decoration_basic_manager.h"
#include "decoration_adapter.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace msh = mir::shell;
namespace msd = msh::decoration;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mrs = mir::renderer::software;
namespace geometry = mir::geometry;
namespace geom = mir::geometry;

auto const buffer_format = mir_pixel_format_argb_8888;
using DeviceEvent = msd::DeviceEvent;

struct InputResolverAdapter: public msd::InputResolver
{
    std::function<void(DeviceEvent& device) > on_process_enter;
    std::function<void() > on_process_leave;
    std::function<void() > on_process_down;
    std::function<void() > on_process_up;
    std::function<void(DeviceEvent& device) > on_process_move;
    std::function<void(DeviceEvent& device) > on_process_drag;

    void process_enter(DeviceEvent& device) override
    {
        on_process_enter(device);
    }
    void process_leave() override
    {
        on_process_leave();
    }
    void process_down() override
    {
        on_process_down();
    }
    void process_up() override
    {
        on_process_up();
    }
    void process_move(DeviceEvent& device) override
    {
        on_process_move(device);
    }
    void process_drag(DeviceEvent& device) override
    {
        on_process_drag(device);
    }
};

struct miral::Decoration::Self
{
    std::function<void(Buffer, mir::geometry::Size)> render_titlebar;
    std::function<void(Buffer, mir::geometry::Size)> render_left_border;
    std::function<void(Buffer, mir::geometry::Size)> render_right_border;
    std::function<void(Buffer, mir::geometry::Size)> render_bottom_border;

    std::function<void(DeviceEvent& device) > on_process_enter;
    std::function<void() > on_process_leave;
    std::function<void() > on_process_down;
    std::function<void() > on_process_up;
    std::function<void(DeviceEvent& device) > on_process_move;
    std::function<void(DeviceEvent& device) > on_process_drag;

    Self() = delete;

    Self(
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
        std::function<void(Buffer, mir::geometry::Size)> render_left_border,
        std::function<void(Buffer, mir::geometry::Size)> render_right_border,
        std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        std::function<void(DeviceEvent& device)> process_drag) :
        render_titlebar{render_titlebar},
        render_left_border{render_left_border},
        render_right_border{render_right_border},
        render_bottom_border{render_bottom_border},
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag}
    {

    }

    // TODO: miral::InputResolver or similar
    std::optional<std::shared_ptr<msd::InputResolver>> input_resolver;
};


miral::Decoration::Decoration(
    std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
    std::function<void(Buffer, mir::geometry::Size)> render_left_border,
    std::function<void(Buffer, mir::geometry::Size)> render_right_border,
    std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

    std::function<void(DeviceEvent& device)> process_enter,
    std::function<void()> process_leave,
    std::function<void()> process_down,
    std::function<void()> process_up,
    std::function<void(DeviceEvent& device)> process_move,
    std::function<void(DeviceEvent& device)> process_drag) :
    self{std::make_unique<Self>(
        render_titlebar,
        render_left_border,
        render_right_border,
        render_bottom_border,
        process_enter,
        process_leave,
        process_down,
        process_up,
        process_move,
        process_drag)}
{
}

inline auto area(geometry::Size size) -> size_t
{
    return (size.width > geom::Width{} && size.height > geom::Height{})
        ? size.width.as_int() * size.height.as_int()
        : 0;
}

class Renderer
{
public:
    Renderer(std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator) :
        buffer_allocator{buffer_allocator}
    {
    }

    auto make_buffer(uint32_t const* pixels, geometry::Size size) -> std::optional<std::shared_ptr<mg::Buffer>>
    {
        if (!area(size))
        {
            /* log_warning("Failed to draw SSD: tried to create zero size buffer"); */
            return std::nullopt;
        }

        try
        {
            return mrs::alloc_buffer_with_content(
                *buffer_allocator,
                reinterpret_cast<unsigned char const*>(pixels),
                size,
                geometry::Stride{size.width.as_uint32_t() * MIR_BYTES_PER_PIXEL(buffer_format)},
                buffer_format);
        }
        catch (std::runtime_error const&)
        {
            /* log_warning("Failed to draw SSD: software buffer not a pixel source"); */
            return std::nullopt;
        }
    }

    static inline void render_row(
        uint32_t* const data,
        geometry::Size buf_size,
        geometry::Point left,
        geometry::Width length,
        uint32_t color)
    {
        if (left.y < geometry::Y{} || left.y >= as_y(buf_size.height))
            return;
        geometry::X const right = std::min(left.x + as_delta(length), as_x(buf_size.width));
        left.x = std::max(left.x, geometry::X{});
        uint32_t* const start = data + (left.y.as_int() * buf_size.width.as_int()) + left.x.as_int();
        uint32_t* const end = start + right.as_int() - left.x.as_int();
        for (uint32_t* i = start; i < end; i++)
            *i = color;
    }


private:
    std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator;

    using Pixel = miral::Decoration::Pixel;
    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr
};

auto miral::Decoration::create_manager()
    -> std::shared_ptr<miral::DecorationManagerAdapter>
{

    auto renderer = std::make_shared<Renderer>(nullptr);
    miral::Decoration deco(
        [](Decoration::Buffer titlebar_pixels, geometry::Size scaled_titlebar_size)
        {
            for (geometry::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geometry::DeltaY{1})
            {
                Renderer::render_row(
                    titlebar_pixels.get(), scaled_titlebar_size, {0, y}, scaled_titlebar_size.width, 0xFF00FFFF);
            }
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        },
        [](auto...)
        {
        });

    return DecorationBasicManager(
               [](auto, auto)
               {
                   return nullptr;
               })
        .to_adapter();
}

// Bring up BasicManager so we can hook this up to be instantiated: miral::BasicManager
// Figure out where to put the InputResolver, preferable somewhere where we have access to the window surface
// Need to also listen to window surface events as well and re-render
