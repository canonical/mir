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

// TODO: Add a builder to provide API stability

// TODO one more layer of indirection
#include "mir/geometry/forward.h"
#include "mir/geometry/size.h"
#include "mir/geometry/point.h"
#include "mir_toolkit/common.h"

#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"

#include <functional>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <optional>


auto const buffer_format = mir_pixel_format_argb_8888;
auto const bytes_per_pixel = 4;

namespace mir
{
namespace input
{
class CursorImages;
}
namespace shell
{
class SurfaceSpecification;
namespace decoration
{
class Decoration;
}
}
namespace scene
{
class Session;
}
namespace compositor
{
class BufferStream;
}
namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
}
}

namespace msh = mir::shell;
namespace msd = msh::decoration;
namespace ms = mir::scene;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geometry = mir::geometry;
namespace geom = mir::geometry;

struct MirEvent;
struct WindowState;

namespace miral
{
struct InputContext;
using OnProcessDrag = std::function<void(miral::DeviceEvent device, InputContext ctx)>;
using OnProcessEnter = std::function<void(miral::DeviceEvent device, InputContext ctx)>;
using OnProcessLeave = std::function<void(InputContext ctx)>;
using OnProcessMove = std::function<void(miral::DeviceEvent device)>;

struct InputContext
{
    InputContext(
        std::shared_ptr<msh::Shell> shell,
        std::shared_ptr<ms::Session> session,
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<const MirEvent> const latest_event,
        std::shared_ptr<mir::input::CursorImages> const cursor_images,
        std::shared_ptr<ms::Surface> const decoration_surface) :
        shell{shell},
        session{session},
        window_surface{window_surface},
        latest_event{latest_event},
        cursor_images{cursor_images},
        decoration_surface{decoration_surface}
    {
    }

    void request_move();

    void request_resize(MirResizeEdge edge);

    void set_cursor(std::string const& cursor_image_name);
private:
    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<ms::Surface> const window_surface;
    std::shared_ptr<const MirEvent> const latest_event;
    std::shared_ptr<mir::input::CursorImages> const cursor_images;
    std::shared_ptr<ms::Surface> const decoration_surface;
};


struct InputResolverAdapter
{
    InputResolverAdapter(
        std::shared_ptr<ms::Surface> decoration_surface,
        std::shared_ptr<msh::Shell> const shell,
        std::shared_ptr<ms::Session> const session,
        std::shared_ptr<ms::Surface> const window_surface,
        std::shared_ptr<mir::input::CursorImages> const cursor_images,
        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag);

    struct Impl;
    std::unique_ptr<Impl> impl;
};

class Renderer
{
public:
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
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar);

    inline auto area(geometry::Size size) -> size_t
    {
        return (size.width > geom::Width{} && size.height > geom::Height{}) ?
                   size.width.as_int() * size.height.as_int() :
                   0;
    }

    auto make_buffer(uint32_t const* pixels, geometry::Size size) -> std::optional<std::shared_ptr<mg::Buffer>>;

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

    void update_state(WindowState const& window_state);

    auto streams_to_spec(std::shared_ptr<WindowState> window_state) -> msh::SurfaceSpecification;

    auto update_render_submit(std::shared_ptr<WindowState> window_state);

    std::shared_ptr<ms::Session> session;
    std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator;
    std::unique_ptr<BufferStreams> buffer_streams;

    geometry::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    std::function<void(Buffer, mir::geometry::Size)> render_titlebar;
};

struct DecorationAdapter
{
    using Buffer = miral::Buffer;

    DecorationAdapter(
        std::shared_ptr<DecorationRedrawNotifier> redraw_notifier,
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
        std::function<void(Buffer, mir::geometry::Size)> render_left_border,
        std::function<void(Buffer, mir::geometry::Size)> render_right_border,
        std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag,
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed,
        std::function<void(std::shared_ptr<WindowState>)> update_decoration_window_state);

    std::shared_ptr<msd::Decoration> to_decoration() const;

    void init(
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<ms::Surface> decoration_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator,
        std::shared_ptr<msh::Shell> shell,
        std::shared_ptr<mir::input::CursorImages> const cursor_images
    );

    void update();

    auto redraw_notifier() -> std::shared_ptr<DecorationRedrawNotifier>;

    struct Impl;
    std::shared_ptr<Impl> impl;
};

}
#endif
