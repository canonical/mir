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
#include "mir_toolkit/common.h"

#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"
#include "miral/decoration_window_state.h"

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
using OnProcessMove = std::function<void(miral::DeviceEvent device, InputContext ctx)>;
using OnProcessUp = std::function<void(InputContext ctx)>;

struct InputContext
{
    InputContext(
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<const MirEvent> const& latest_event,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images,
        std::shared_ptr<ms::Surface> const& decoration_surface) :
        shell{shell},
        session{session},
        window_surface{window_surface},
        latest_event{latest_event},
        cursor_images{cursor_images},
        decoration_surface{decoration_surface}
    {
    }

    void request_move() const;

    void request_resize(MirResizeEdge edge) const;

    void set_cursor(std::string const& cursor_image_name) const;

    void request_close() const;

    void request_toggle_maximize() const;

    void request_minimize() const;
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
        std::shared_ptr<ms::Surface> const& decoration_surface,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images,
        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        std::function<void()> process_down,
        OnProcessUp process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag);

    struct Impl;
    std::unique_ptr<Impl> const impl;
};

class Renderer
{
public:
    static inline auto area(geometry::Size const size) -> size_t
    {
        return (size.width > geom::Width{} && size.height > geom::Height{}) ?
                   size.width.as_int() * size.height.as_int() :
                   0;
    }

    struct Buffer {
        Buffer(std::shared_ptr<ms::Session> const& session);
        ~Buffer();

        auto size() const -> geometry::Size const { return size_; }

        auto get() const -> Pixel*
        {
            return pixels_.get();
        }

        void resize(geometry::Size new_size)
        {
            size_ = new_size;
            pixels_ = std::unique_ptr<Pixel[]>{new uint32_t[area(new_size) * bytes_per_pixel]};
        }

        auto stream() const -> std::shared_ptr<mc::BufferStream>
        {
            return stream_;
        }

    private:
        Buffer(Buffer const&) = delete;
        Buffer& operator=(Buffer const&) = delete;

        geometry::Size size_{};
        // can be nullptr, but should never be nullptr when passed to user
        // rendering callbacks
        std::unique_ptr<Pixel[]> pixels_;

        std::shared_ptr<ms::Session> const session;
        std::shared_ptr<mc::BufferStream> const stream_;
    };

    using DeviceEvent = mir::shell::decoration::DeviceEvent;
    using RenderingCallback = std::function<void(Buffer&)>;

    Renderer(
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        Renderer::RenderingCallback render_titlebar,
        Renderer::RenderingCallback render_left_border,
        Renderer::RenderingCallback render_right_border,
        Renderer::RenderingCallback render_bottom_border);

    static inline void render_row(
        Buffer const& buffer,
        geometry::Point left,
        geometry::Width length,
        uint32_t color)
    {
        auto buf_size = buffer.size();
        if (left.y < geometry::Y{} || left.y >= as_y(buf_size.height))
            return;
        geometry::X const right = std::min(left.x + as_delta(length), as_x(buf_size.width));
        left.x = std::max(left.x, geometry::X{});
        uint32_t* const start = buffer.get() + (left.y.as_int() * buf_size.width.as_int()) + left.x.as_int();
        uint32_t* const end = start + right.as_int() - left.x.as_int();
        for (uint32_t* i = start; i < end; i++)
            *i = color;
    }

    void update_state(WindowState const& window_state);
    auto streams_to_spec(std::shared_ptr<WindowState> const&  window_state) const -> msh::SurfaceSpecification;
    void update_render_submit(std::shared_ptr<WindowState> const& window_state);

private:
    auto make_graphics_buffer(Buffer const&) -> std::optional<std::shared_ptr<mg::Buffer>>;

    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;

    Buffer titlebar_buffer;
    Buffer left_border_buffer;
    Buffer right_border_buffer;
    Buffer bottom_border_buffer;

    RenderingCallback const render_titlebar;
    RenderingCallback const render_left_border;
    RenderingCallback const render_right_border;
    RenderingCallback const render_bottom_border;
};

struct DecorationAdapter
{
    DecorationAdapter(
        std::shared_ptr<DecorationRedrawNotifier> const& redraw_notifier,
        Renderer::RenderingCallback render_titlebar,
        Renderer::RenderingCallback render_left_border,
        Renderer::RenderingCallback render_right_border,
        Renderer::RenderingCallback render_bottom_border,

        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        std::function<void()> process_down,
        OnProcessUp process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag,
        std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int /*value*/)> attrib_changed,
        std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>
            window_resized_to,
        std::function<void(ms::Surface const* window_surface, std::string const& /*name*/)> window_renamed,
        std::function<void(std::shared_ptr<WindowState>)> update_decoration_window_state);

    void set_custom_geometry(std::shared_ptr<StaticGeometry> geometry);

    std::shared_ptr<msd::Decoration> to_decoration() const;

    void init(
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<ms::Surface> const& decoration_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images
    );

    void update();

    auto redraw_notifier() const -> std::shared_ptr<DecorationRedrawNotifier>;

    struct Impl;
    std::shared_ptr<Impl> const impl;
};

}
#endif
