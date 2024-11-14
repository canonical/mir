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


#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/scene/session.h"
#include "mir/renderer/sw/pixel_source.h"
#include "miral/decoration_manager_builder.h"
#include "miral/decoration_basic_manager.h"
#include "mir/server.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/wayland/weak.h"
#include "mir/shell/decoration.h"
#include "decoration_adapter.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace msh = mir::shell;
namespace msd = msh::decoration;
namespace mc = mir::compositor;
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

/// Decoration geometry properties that don't change
struct StaticGeometry
{
    geometry::Height const
        titlebar_height; ///< Visible height of the top titlebar with the window's name and buttons
    geometry::Width const side_border_width;       ///< Visible width of the side borders
    geometry::Height const bottom_border_height;   ///< Visible height of the bottom border
    geometry::Size const resize_corner_input_size; ///< The size of the input area of a resizable corner
                                                   ///< (does not effect surface input area, only where in the
                                                   ///< surface is considered a resize corner)
    geometry::Width const button_width;            ///< The width of window control buttons
    geometry::Width const padding_between_buttons; ///< The gep between titlebar buttons
    geometry::Height const title_font_height;      ///< Height of the text used to render the window title
    geometry::Point const title_font_top_left;     ///< Where to render the window title
    geometry::Displacement const icon_padding;     ///< Padding inside buttons around icons
    geometry::Width const icon_line_width;         ///< Width for lines in button icons
};

class WindowState
{
public:
    WindowState(
        std::shared_ptr<StaticGeometry const> const& static_geometry,
        std::shared_ptr<ms::Surface> const& window_surface,
        float scale);

    auto window_size() const -> geometry::Size;
    auto focused_state() const -> MirWindowFocusState;
    auto window_name() const -> std::string;

    auto titlebar_width() const -> geometry::Width;
    auto titlebar_height() const -> geometry::Height;

    auto titlebar_rect() const -> geometry::Rectangle;

private:
    WindowState(WindowState const&) = delete;
    WindowState& operator=(WindowState const&) = delete;

    std::shared_ptr<StaticGeometry const> const static_geometry;
    geometry::Size const window_size_;
    MirWindowFocusState const focus_state_;
    std::string window_name_;
    float scale_;
};

auto WindowState::titlebar_rect() const -> geom::Rectangle
{
    return {
        geom::Point{},
        {titlebar_width(), titlebar_height()}};
}

auto WindowState::titlebar_width() const -> geom::Width
{
    return window_size().width;
}

auto WindowState::titlebar_height() const -> geom::Height
{
    return static_geometry->titlebar_height;
}

inline auto area(geometry::Size size) -> size_t
{
    return (size.width > geom::Width{} && size.height > geom::Height{})
        ? size.width.as_int() * size.height.as_int()
        : 0;
}

auto WindowState::window_size() const -> geom::Size
{
    return window_size_;
}

auto WindowState::focused_state() const -> MirWindowFocusState
{
    return focus_state_;
}

auto WindowState::window_name() const -> std::string
{
    return window_name_;
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

    void update_state(WindowState const& window_state)
    {
        if (window_state.titlebar_rect().size != titlebar_size)
        {
            titlebar_size = window_state.titlebar_rect().size;
            titlebar_pixels.reset(); // force a reallocation next time it's needed
        }
    }

    std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator;

    using Pixel = miral::Decoration::Pixel;
    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr
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

    class BufferStreams
    {
        // Must be at top so it can be used by create_buffer_stream() when called in the constructor
        std::shared_ptr<ms::Session> const session;

    public:
        BufferStreams(std::shared_ptr<ms::Session> const& session);
        ~BufferStreams();

        auto create_buffer_stream() -> std::shared_ptr<mc::BufferStream>;

        std::shared_ptr<mc::BufferStream> const titlebar;
        std::shared_ptr<mc::BufferStream> const left_border;
        std::shared_ptr<mc::BufferStream> const right_border;
        std::shared_ptr<mc::BufferStream> const bottom_border;

    private:
        BufferStreams(BufferStreams const&) = delete;
        BufferStreams& operator=(BufferStreams const&) = delete;
    };

    // TODO: miral::InputResolver or similar
    std::optional<std::shared_ptr<msd::InputResolver>> input_resolver;
    std::unique_ptr<BufferStreams> buffer_streams;
    std::unique_ptr<WindowState> window_state;
    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<ms::Surface> const decoration_surface;

    std::unique_ptr<Renderer> renderer;
};

miral::Decoration::Self::BufferStreams::BufferStreams(std::shared_ptr<ms::Session> const& session)
    : session{session},
      titlebar{create_buffer_stream()},
      left_border{create_buffer_stream()},
      right_border{create_buffer_stream()},
      bottom_border{create_buffer_stream()}
{
}

miral::Decoration::Self::BufferStreams::~BufferStreams()
{
    session->destroy_buffer_stream(titlebar);
    session->destroy_buffer_stream(left_border);
    session->destroy_buffer_stream(right_border);
    session->destroy_buffer_stream(bottom_border);
}

auto miral::Decoration::Self::BufferStreams::create_buffer_stream() -> std::shared_ptr<mc::BufferStream>
{
    auto const stream = session->create_buffer_stream(mg::BufferProperties{
        geom::Size{1, 1},
        buffer_format,
        mg::BufferUsage::software});
    return stream;
}

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

void miral::Decoration::render()
{
    using StreamSpecification = mir::shell::StreamSpecification;
    msh::SurfaceSpecification spec;

    spec.streams = std::vector<StreamSpecification>{};
    auto const emplace = [&](std::shared_ptr<mc::BufferStream> stream, geom::Rectangle rect)
        {
            if (rect.size.width > geom::Width{} && rect.size.height > geom::Height{})
                spec.streams.value().emplace_back(StreamSpecification{stream, as_displacement(rect.top_left)});
        };

    auto& buffer_streams = self->buffer_streams;
    auto& window_state = self->window_state;
    auto& shell = self->shell;
    auto& session = self->session;
    auto& decoration_surface = self->decoration_surface;
    auto& renderer = self->renderer;

    emplace(buffer_streams->titlebar, window_state->titlebar_rect());

    if (!spec.is_empty())
    {
        shell->modify_surface(session, decoration_surface, spec);
    }

    renderer->update_state(*window_state);

    std::vector<std::pair<
        std::shared_ptr<mc::BufferStream>,
        std::optional<std::shared_ptr<mg::Buffer>>>> new_buffers;

    self->render_titlebar(renderer->titlebar_pixels.get(), renderer->titlebar_size);
    new_buffers.emplace_back(
        buffer_streams->titlebar,
        renderer->make_buffer(renderer->titlebar_pixels.get(), renderer->titlebar_size));

    float inv_scale = 1.0f; // 1.0f / window_state->scale();
    for (auto const& pair : new_buffers)
    {
        if (pair.second)
            pair.first->submit_buffer(
                pair.second.value(),
                pair.second.value()->size() * inv_scale,
                {{0, 0}, geom::SizeD{pair.second.value()->size()}});
    }
}

auto miral::Decoration::create_manager()
    -> std::shared_ptr<miral::DecorationManagerAdapter>
{
    miral::Decoration deco(
        [](Decoration::Buffer titlebar_pixels, geometry::Size scaled_titlebar_size)
        {
            for (geometry::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geometry::DeltaY{1})
            {
                Renderer::render_row(
                    titlebar_pixels, scaled_titlebar_size, {0, y}, scaled_titlebar_size.width, 0xFF00FFFF);
            }
        },
        [](auto...)
        {
            // Need to wire up input events
            // On any input event -> Decoration::Render
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
