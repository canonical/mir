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

#ifndef MIRAL_DECORATION_DECORATION_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_ADAPTER_H

/* #include "mir/shell/decoration_notifier.h" */

#include "decoration_window_state.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/server.h"
#include "mir/shell/decoration.h"
#include "mir/shell/decoration/input_resolver.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "miral/decoration.h"
#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"

#include <functional>
#include <glm/gtc/constants.hpp>
#include <memory>

namespace msh = mir::shell;
namespace msd = msh::decoration;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mrs = mir::renderer::software;
namespace geometry = mir::geometry;
namespace geom = mir::geometry;

auto const buffer_format = mir_pixel_format_argb_8888;
auto const bytes_per_pixel = 4;


struct MirEvent;

namespace miral
{
using DeviceEvent = msd::DeviceEvent;
struct InputContext;
using OnProcessDrag = std::function<void(DeviceEvent& device, InputContext ctx)>;

struct InputContext
{
    InputContext(
        std::shared_ptr<msh::Shell> shell,
        std::shared_ptr<ms::Session> session,
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<const MirEvent> const latest_event) :
        shell{shell},
        session{session},
        window_surface{window_surface},
        latest_event{latest_event}
    {
    }

    void request_move()
    {
        shell->request_move(session, window_surface, mir_event_get_input_event(latest_event.get()));
    }

private:
    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<ms::Surface> const window_surface;
    std::shared_ptr<const MirEvent> const latest_event;
};


struct InputResolverAdapter: public msd::InputResolver
{
    InputResolverAdapter(
        std::shared_ptr<ms::Surface> decoration_surface,
        std::shared_ptr<msh::Shell> const shell,
        std::shared_ptr<ms::Session> const session,
        std::shared_ptr<ms::Surface> const window_surface,
        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        OnProcessDrag process_drag) :
        msd::InputResolver(decoration_surface),
        shell{shell},
        session{session},
        window_surface{window_surface},
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag}
    {
    }

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
        on_process_drag(device, InputContext(shell, session, window_surface, latest_event()));
    }

    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<ms::Surface> const window_surface;

    std::function<void(DeviceEvent& device)> on_process_enter;
    std::function<void()> on_process_leave;
    std::function<void()> on_process_down;
    std::function<void()> on_process_up;
    std::function<void(DeviceEvent& device)> on_process_move;
    OnProcessDrag on_process_drag;
};

class WindowSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    WindowSurfaceObserver(
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed) :
        on_attrib_changed{attrib_changed},
        on_window_resized_to(window_resized_to),
        on_window_renamed(window_renamed)
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void attrib_changed(ms::Surface const* window_surface, MirWindowAttrib attrib, int value) override
    {
        switch(attrib)
        {
        case mir_window_attrib_type:
        case mir_window_attrib_state:
        case mir_window_attrib_focus:
        case mir_window_attrib_visibility:
            on_attrib_changed(window_surface, attrib, value);
            break;

        case mir_window_attrib_dpi:
        case mir_window_attrib_preferred_orientation:
        case mir_window_attribs:
            break;
        }
    }

    void window_resized_to(ms::Surface const* window_surface, mir::geometry::Size const& window_size) override
    {
        on_window_resized_to(window_surface, window_size);
    }

    void renamed(ms::Surface const* window_surface, std::string const& name) override
    {
        on_window_renamed(window_surface, name);
    }
    /// @}

private:
    std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> on_attrib_changed;
    std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
        on_window_resized_to;
    std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> on_window_renamed;
};

class Renderer
{
public:
    using Pixel = uint32_t;
    using Buffer = Pixel*;
    using DeviceEvent = mir::shell::decoration::DeviceEvent;

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

    Renderer(
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar) :
        session{window_surface->session().lock()},
        buffer_allocator{buffer_allocator},
        buffer_streams{std::make_unique<BufferStreams>(session)},
        render_titlebar{render_titlebar}
    {
    }

    inline auto area(geometry::Size size) -> size_t
    {
        return (size.width > geom::Width{} && size.height > geom::Height{}) ?
                   size.width.as_int() * size.height.as_int() :
                   0;
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
            titlebar_pixels = std::unique_ptr<uint32_t[]>{new uint32_t[area(titlebar_size) * bytes_per_pixel]};
        }
    }

    auto streams_to_spec(std::shared_ptr<WindowState> window_state) -> msh::SurfaceSpecification;

    auto update_render_submit(std::shared_ptr<WindowState> window_state);

    std::shared_ptr<ms::Session> session;
    std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator;
    std::unique_ptr<BufferStreams> buffer_streams;

    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    std::function<void(Buffer, mir::geometry::Size)> render_titlebar;
};

class DecorationAdapter : public mir::shell::decoration::Decoration
{
public:
    using Buffer = miral::Renderer::Buffer;
    using DeviceEvent = miral::Renderer::DeviceEvent;

    DecorationAdapter() = delete;

    DecorationAdapter(
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
        std::function<void(Buffer, mir::geometry::Size)> render_left_border,
        std::function<void(Buffer, mir::geometry::Size)> render_right_border,
        std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        OnProcessDrag process_drag,
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed,
        std::function<void(std::shared_ptr<WindowState>)> update_decoration_window_state) :
        render_titlebar{render_titlebar},
        render_left_border{render_left_border},
        render_right_border{render_right_border},
        render_bottom_border{render_bottom_border},
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag},
        on_update_decoration_window_state{update_decoration_window_state},
        on_attrib_changed{[this, attrib_changed](auto... args)
                          {
                              window_state_updated(window_surface);
                              attrib_changed(args...);
                              on_update_decoration_window_state(window_state);
                          }},
        on_window_resized_to{[this, window_resized_to](auto... args)
                             {
                                 window_state_updated(window_surface);
                                 window_resized_to(args...);
                              on_update_decoration_window_state(window_state);
                             }},
        on_window_renamed{[this, window_renamed](auto... args)
                          {
                              window_state_updated(window_surface);
                              window_renamed(args...);
                              on_update_decoration_window_state(window_state);
                          }}
    {
    }

    void set_scale(float) override
    {
    }

    void init(
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<ms::Surface> decoration_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator,
        std::shared_ptr<msh::Shell> shell
    )
    {
        this->window_surface = window_surface;
        this->shell = shell;
        this->session = window_surface->session().lock();
        this->decoration_surface = decoration_surface;
        this->window_state = std::make_shared<WindowState>(default_geometry, window_surface.get());

        renderer = std::make_unique<Renderer>(window_surface, buffer_allocator, render_titlebar);
        input_adapter = std::make_unique<InputResolverAdapter>(
            decoration_surface,
            shell,
            session,
            window_surface,
            on_process_enter,
            on_process_leave,
            on_process_down,
            on_process_up,
            on_process_move,
            on_process_drag);

        window_surface_observer =
            std::make_shared<WindowSurfaceObserver>(on_attrib_changed, on_window_resized_to, on_window_renamed);
        window_surface->register_interest(window_surface_observer);

        // Initialize widget rects
        on_update_decoration_window_state(window_state);
    }

    void update();

    StaticGeometry const default_geometry{
        geom::Height{24},         // titlebar_height
        geom::Width{6},           // side_border_width
        geom::Height{6},          // bottom_border_height
        geom::Size{16, 16},       // resize_corner_input_size
        geom::Width{24},          // button_width
        geom::Width{6},           // padding_between_buttons
        geom::Height{14},         // title_font_height
        geom::Point{8, 2},        // title_font_top_left
        geom::Displacement{5, 5}, // icon_padding
        geom::Width{1},           // detail_line_width
    };

    void window_state_updated(std::shared_ptr<ms::Surface> const window_surface)
    {
        window_state = std::make_shared<WindowState>(default_geometry, window_surface.get());
    }

    ~DecorationAdapter()
    {
        window_surface->unregister_interest(*window_surface_observer);
    }

private:
    std::unique_ptr<InputResolverAdapter> input_adapter;
    std::shared_ptr<WindowSurfaceObserver> window_surface_observer;
    std::shared_ptr<ms::Surface> window_surface;
    std::shared_ptr<ms::Surface> decoration_surface;
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<ms::Session> session;
    std::shared_ptr<WindowState> window_state;

    std::function<void(Buffer, mir::geometry::Size)> render_titlebar;
    std::function<void(Buffer, mir::geometry::Size)> render_left_border;
    std::function<void(Buffer, mir::geometry::Size)> render_right_border;
    std::function<void(Buffer, mir::geometry::Size)> render_bottom_border;

    std::function<void(DeviceEvent& device)> on_process_enter;
    std::function<void()> on_process_leave;
    std::function<void()> on_process_down;
    std::function<void()> on_process_up;
    std::function<void(DeviceEvent& device)> on_process_move;
    OnProcessDrag on_process_drag;

    std::function<void(std::shared_ptr<WindowState>)> on_update_decoration_window_state;
    std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> on_attrib_changed;
    std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
        on_window_resized_to;
    std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> on_window_renamed;
};

}
#endif
