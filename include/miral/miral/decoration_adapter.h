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

#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"

#include "miral/decoration_basic_manager.h"
#include "miral/decoration_window_state.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>



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

struct MirEvent;

namespace miral
{
namespace decoration
{
class WindowState;
class StaticGeometry;
class InputContext;

using OnProcessDrag = std::function<void(miral::DeviceEvent device, miral::decoration::InputContext ctx)>;
using OnProcessEnter = std::function<void(miral::DeviceEvent device, miral::decoration::InputContext ctx)>;
using OnProcessLeave = std::function<void(miral::decoration::InputContext ctx)>;
using OnProcessMove = std::function<void(miral::DeviceEvent device, miral::decoration::InputContext ctx)>;
using OnProcessUp = std::function<void(miral::decoration::InputContext ctx)>;
using OnProcessDown = std::function<void()>;

using OnWindowAttribChanged = std::function<void(ms::Surface const* window_surface, MirWindowAttrib attrib, int value)>;
using OnWindowResized =
    std::function<void(ms::Surface const* window_surface, mir::geometry::Size const& /*window_size*/)>;
using OnWindowRenamed = std::function<void(ms::Surface const* window_surface, std::string const& name)>;
using OnWindowStateUpdated = std::function<void(std::shared_ptr<miral::decoration::WindowState>)>;
using OnScaleChanged = std::function<void(float)>;

struct InputContext
{
    InputContext(
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<MirEvent const> const& latest_event,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images,
        std::shared_ptr<ms::Surface> const& decoration_surface);

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


class Renderer
{
public:
    static auto area(geometry::Size const size) -> size_t;

    using Pixel = uint32_t;

    // Can we use even more keywords?
    static inline auto const buffer_format = mir_pixel_format_argb_8888;
    static inline auto const bytes_per_pixel = 4;


    struct Buffer {
        Buffer(std::shared_ptr<ms::Session> const& session);

        ~Buffer();

        auto size() const -> geometry::Size;

        auto get() const -> Pixel*;

        void resize(geometry::Size new_size);

        auto stream() const -> std::shared_ptr<mc::BufferStream>;

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

    using RenderingCallback = std::function<void(Buffer&)>;

    Renderer(
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        Renderer::RenderingCallback render_titlebar,
        Renderer::RenderingCallback render_left_border,
        Renderer::RenderingCallback render_right_border,
        Renderer::RenderingCallback render_bottom_border);

    static void fill_solid_color(Renderer::Buffer const& left_border_buffer, Renderer::Pixel color);

    // Renders in display-scale-space, takes into account the display scale
    static void render_rect_scaled(Buffer const& buffer, geometry::Rectangle unscaled_rect, float scale, Pixel color);

    void update_state(miral::decoration::WindowState const& window_state);
    auto streams_to_spec(std::shared_ptr<miral::decoration::WindowState> const& window_state) const
        -> msh::SurfaceSpecification;
    void update_render_submit(std::shared_ptr<miral::decoration::WindowState> const& window_state);

    void scale_changed(float new_scale);
    auto scale() const -> float;

private:
    auto make_graphics_buffer(Buffer const&) -> std::optional<std::shared_ptr<mg::Buffer>>;

    // Renders in buffer pixel-scale-space, independent of the actual display scale
    static void render_row_unscaled(Buffer const& buffer, geometry::Point left, geometry::Width length, uint32_t color);


    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;

    Buffer titlebar_buffer;
    Buffer left_border_buffer;
    Buffer right_border_buffer;
    Buffer bottom_border_buffer;

    RenderingCallback const render_titlebar;
    RenderingCallback const render_left_border;
    RenderingCallback const render_right_border;
    RenderingCallback const render_bottom_border;

    float scale_ = 1.0;
};

struct DecorationAdapter
{
    DecorationAdapter(
        std::shared_ptr<DecorationRedrawNotifier> const& redraw_notifier,

        // Called by user code in response to input events
        Renderer::RenderingCallback render_titlebar,
        Renderer::RenderingCallback render_left_border,
        Renderer::RenderingCallback render_right_border,
        Renderer::RenderingCallback render_bottom_border,

        // Called by Mir to inform user code of input events
        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        OnProcessDown process_down,
        OnProcessUp process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag,

        // Called by Mir to inform user code of changes to the window being decorated
        OnWindowAttribChanged attrib_changed,
        OnWindowResized window_resized_to,
        OnWindowRenamed window_renamed,

        // Called by Mir to inform user code that the display scale changed
        OnScaleChanged scale_changed,

        // Called whenever the internal WindowState is updated
        OnWindowStateUpdated update_decoration_window_state);

    void set_custom_geometry(std::shared_ptr<miral::decoration::StaticGeometry> geometry);

    auto to_decoration() -> std::unique_ptr<msd::Decoration>;

    void init(
        std::shared_ptr<ms::Surface> const& window_surface,
        std::shared_ptr<ms::Surface> const& decoration_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images
    );

private:
    struct Self;
    std::shared_ptr<Self> self;
};

}
}
#endif
