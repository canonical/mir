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

#include "miral/decoration_adapter.h"

#include "mir_toolkit/events/event.h"
#include "mir/geometry/forward.h"
#include "miral/decoration_window_state.h"
#include "miral/decoration.h"
#include "miral/decoration_manager_builder.h"

#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/input/cursor_images.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/decoration.h"
#include "mir/shell/decoration/input_resolver.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/log.h"

#include <memory>

namespace msh = mir::shell;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mrs = mir::renderer::software;
namespace geometry = mir::geometry;
namespace geom = mir::geometry;

namespace md = miral::decoration;

miral::InputContext::InputContext(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& window_surface,
    std::shared_ptr<MirEvent const> const& latest_event,
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

void miral::InputContext::request_move() const
{
    shell->request_move(session, window_surface, mir_event_get_input_event(latest_event.get()));
}

void miral::InputContext::request_resize(MirResizeEdge edge) const
{
    shell->request_resize(session, window_surface, mir_event_get_input_event(latest_event.get()), edge);
}

void miral::InputContext::set_cursor(std::string const& cursor_image_name) const
{
    msh::SurfaceSpecification spec;
    // size is hard-coded because current implementation ignores it
    spec.cursor_image = cursor_images->image(cursor_image_name, {16, 16});
    shell->modify_surface(session, decoration_surface, spec);
}

void miral::InputContext::request_close() const{
    window_surface->request_client_surface_close();
}

void miral::InputContext::request_toggle_maximize() const{
    msh::SurfaceSpecification spec;
    if (window_surface->state() == mir_window_state_maximized)
        spec.state = mir_window_state_restored;
    else
        spec.state = mir_window_state_maximized;
    shell->modify_surface(session, window_surface, spec);
}

void miral::InputContext::request_minimize() const{
    msh::SurfaceSpecification spec;
    spec.state = mir_window_state_minimized;
    shell->modify_surface(session, window_surface, spec);
}

struct InputResolverAdapter: public msd::InputResolver
{
    InputResolverAdapter(
        std::shared_ptr<ms::Surface> const decoration_surface,
        std::shared_ptr<msh::Shell> const shell,
        std::shared_ptr<ms::Session> const session,
        std::shared_ptr<ms::Surface> const window_surface,
        std::shared_ptr<mir::input::CursorImages> const cursor_images,
        miral::OnProcessEnter process_enter,
        miral::OnProcessLeave process_leave,
        miral::OnProcessDown process_down,
        miral::OnProcessUp process_up,
        miral::OnProcessMove process_move,
        miral::OnProcessDrag process_drag) :
        msd::InputResolver(decoration_surface),
        shell{shell},
        session{session},
        window_surface{window_surface},
        cursor_images{cursor_images},
        on_process_enter{process_enter},
        on_process_leave{process_leave},
        on_process_down{process_down},
        on_process_up{process_up},
        on_process_move{process_move},
        on_process_drag{process_drag}
    {
    }

    auto input_ctx() -> miral::InputContext
    {
        return miral::InputContext(shell, session, window_surface, latest_event(), cursor_images, decoration_surface);
    }

    void process_enter(msd::DeviceEvent& device) override
    {
        on_process_enter(miral::DeviceEvent(device), input_ctx());
    }

    void process_leave() override
    {
        on_process_leave(input_ctx());
    }

    void process_down() override
    {
        on_process_down();
    }

    void process_up() override
    {
        on_process_up(input_ctx());
    }

    void process_move(msd::DeviceEvent& device) override
    {
        on_process_move(device, input_ctx());
    }

    void process_drag(msd::DeviceEvent& device) override
    {
        on_process_drag(device, input_ctx());
    }

    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::Session> const session;
    std::shared_ptr<ms::Surface> const window_surface;
    std::shared_ptr<mir::input::CursorImages> const cursor_images;

    miral::OnProcessEnter const on_process_enter;
    miral::OnProcessLeave const on_process_leave;
    miral::OnProcessDown const on_process_down;
    miral::OnProcessUp const on_process_up;
    miral::OnProcessMove const on_process_move;
    miral::OnProcessDrag const on_process_drag;
};

miral::decoration::StaticGeometry const default_geometry{
    geom::Height{24},         // titlebar_height
    geom::Width{6},           // side_border_width
    geom::Height{6},          // bottom_border_height
    geom::Size{16, 16},       // resize_corner_input_size
    geom::Width{24},          // button_width
    geom::Width{6},           // padding_between_buttons
};

miral::Renderer::Renderer(
    std::shared_ptr<ms::Surface> const& window_surface,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    Renderer::RenderingCallback render_titlebar,
    Renderer::RenderingCallback render_left_border,
    Renderer::RenderingCallback render_right_border,
    Renderer::RenderingCallback render_bottom_border) :
    session{window_surface->session().lock()},
    buffer_allocator{buffer_allocator},
    titlebar_buffer{session},
    left_border_buffer{session},
    right_border_buffer{session},
    bottom_border_buffer{session},
    render_titlebar{render_titlebar},
    render_left_border{render_left_border},
    render_right_border{render_right_border},
    render_bottom_border{render_bottom_border}
{
}

auto miral::Renderer::area(geometry::Size const size) -> size_t
{
    return (size.width > geometry::Width{} && size.height > geometry::Height{}) ?
               size.width.as_int() * size.height.as_int() :
               0;
}

auto miral::Renderer::Buffer::size() const -> geometry::Size
{
    return size_;
}

auto miral::Renderer::Buffer::get() const -> Pixel*
{
    return pixels_.get();
}

void miral::Renderer::Buffer::resize(geometry::Size new_size)
{
    size_ = new_size;
    pixels_ = std::unique_ptr<Pixel[]>{new uint32_t[area(new_size) * bytes_per_pixel]};
}

auto miral::Renderer::Buffer::stream() const -> std::shared_ptr<mc::BufferStream>
{
    return stream_;
}

void miral::Renderer::render_row(Buffer const& buffer, geometry::Point left, geometry::Width length, uint32_t color)
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

auto miral::Renderer::make_graphics_buffer(Buffer const& buffer)
    -> std::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(buffer.size()))
    {
        mir::log_warning("Failed to draw SSD: tried to create zero size buffer");
        return std::nullopt;
    }

    try
    {
        return mrs::alloc_buffer_with_content(
            *buffer_allocator,
            reinterpret_cast<unsigned char const*>(buffer.get()),
            buffer.size(),
            geometry::Stride{buffer.size().width.as_uint32_t() * MIR_BYTES_PER_PIXEL(buffer_format)},
            buffer_format);
    }
    catch (std::runtime_error const&)
    {
        mir::log_warning("Failed to draw SSD: software buffer not a pixel source");
        return std::nullopt;
    }
}

void miral::Renderer::update_state(md::WindowState const& window_state)
{
    auto const conditional_resize =
        [](auto& buffer, auto const& rect)
    {
        if (rect.size != buffer.size())
            buffer.resize(rect.size);
    };

    conditional_resize(titlebar_buffer, window_state.titlebar_rect());
    conditional_resize(left_border_buffer, window_state.left_border_rect());
    conditional_resize(right_border_buffer, window_state.right_border_rect());
    conditional_resize(bottom_border_buffer, window_state.bottom_border_rect());
}

miral::Renderer::Buffer::Buffer(std::shared_ptr<ms::Session> const& session) :
    stream_{
        session->create_buffer_stream(mg::BufferProperties{geom::Size{1, 1}, buffer_format, mg::BufferUsage::software})}
{
}

miral::Renderer::Buffer::~Buffer()
{
    session->destroy_buffer_stream(stream_);
}

auto miral::Renderer::streams_to_spec(std::shared_ptr<md::WindowState> const& window_state) const
    -> msh::SurfaceSpecification
{
    using StreamSpecification = mir::shell::StreamSpecification;

    msh::SurfaceSpecification spec;
    spec.streams = std::vector<StreamSpecification>{};

    auto const emplace = [&](std::shared_ptr<mc::BufferStream> stream, geom::Rectangle rect)
    {
        if (rect.size.width > geom::Width{} && rect.size.height > geom::Height{})
            spec.streams.value().emplace_back(StreamSpecification{stream, as_displacement(rect.top_left)});
    };

    using md::BorderType;
    switch(window_state->border_type())
    {
    case BorderType::Full:
        emplace(titlebar_buffer.stream(), window_state->titlebar_rect());
        emplace(left_border_buffer.stream(), window_state->left_border_rect());
        emplace(right_border_buffer.stream(), window_state->right_border_rect());
        emplace(bottom_border_buffer.stream(), window_state->bottom_border_rect());
        break;
    case BorderType::Titlebar:
        emplace(titlebar_buffer.stream(), window_state->titlebar_rect());
        break;
    case BorderType::None:
        break;
    }

    return spec;
}

void miral::Renderer::update_render_submit(std::shared_ptr<md::WindowState> const& window_state)
{
    update_state(*window_state);

    std::vector<std::pair<std::shared_ptr<mc::BufferStream>, std::optional<std::shared_ptr<mg::Buffer>>>> new_buffers;
    using md::BorderType;
    switch(window_state->border_type())
    {
    case BorderType::Full:
        render_titlebar(titlebar_buffer);
        render_left_border(left_border_buffer);
        render_right_border(right_border_buffer);
        render_bottom_border(bottom_border_buffer);
        new_buffers.emplace_back(titlebar_buffer.stream(), make_graphics_buffer(titlebar_buffer));
        new_buffers.emplace_back(left_border_buffer.stream(), make_graphics_buffer(left_border_buffer));
        new_buffers.emplace_back(right_border_buffer.stream(), make_graphics_buffer(right_border_buffer));
        new_buffers.emplace_back(bottom_border_buffer.stream(), make_graphics_buffer(bottom_border_buffer));
        break;
    case BorderType::Titlebar:
        render_titlebar(titlebar_buffer);
        new_buffers.emplace_back(titlebar_buffer.stream(), make_graphics_buffer(titlebar_buffer));
        break;
    case BorderType::None:
        break;
    }

    float inv_scale = 1.0f; // 1.0f / window_state->scale();
    for (auto const& [stream, buffer_opt] : new_buffers)
    {
        if (buffer_opt)
        {
            auto& buffer = buffer_opt.value();
            stream->submit_buffer(buffer, buffer->size() * inv_scale, {{0, 0}, geom::SizeD{buffer->size()}});
        }
    }
}

class WindowSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    WindowSurfaceObserver(
        miral::OnWindowAttribChanged attrib_changed,
        miral::OnWindowResized window_resized_to,
        miral::OnWindowRenamed window_renamed) :
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
    miral::OnWindowAttribChanged const on_attrib_changed;
    miral::OnWindowResized const on_window_resized_to;
    miral::OnWindowRenamed const on_window_renamed;
};

struct miral::DecorationAdapter::Impl : public msd::Decoration
{
public:
    Impl() = delete;

    Impl(
        std::shared_ptr<DecorationRedrawNotifier> redraw_notifier,
        Renderer::RenderingCallback render_titlebar,
        Renderer::RenderingCallback render_left_border,
        Renderer::RenderingCallback render_right_border,
        Renderer::RenderingCallback render_bottom_border,
        OnProcessEnter process_enter,
        OnProcessLeave process_leave,
        OnProcessDown process_down,
        OnProcessUp process_up,
        OnProcessMove process_move,
        OnProcessDrag process_drag,
        OnWindowAttribChanged attrib_changed,
        OnWindowResized window_resized_to,
        OnWindowRenamed window_renamed,
        OnWindowStateUpdated update_decoration_window_state) :
        redraw_notifier_{redraw_notifier},
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
                          }},
        geometry{std::make_shared<md::StaticGeometry>(default_geometry)}
    {
    }

    void set_custom_geometry(std::shared_ptr<md::StaticGeometry> geometry);

    void init(
        std::shared_ptr<ms::Surface> window_surface,
        std::shared_ptr<ms::Surface> decoration_surface,
        std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator,
        std::shared_ptr<msh::Shell> shell,
        std::shared_ptr<mir::input::CursorImages> const cursor_images);

    void update();

    void window_state_updated(std::shared_ptr<ms::Surface> const window_surface);

    auto redraw_notifier() { return redraw_notifier_; }

    void set_scale(float) override {}

    ~Impl();

private:

    std::shared_ptr<DecorationRedrawNotifier> const redraw_notifier_;
    std::unique_ptr<InputResolverAdapter> input_adapter;
    std::shared_ptr<WindowSurfaceObserver> window_surface_observer;
    std::shared_ptr<ms::Surface> window_surface;
    std::shared_ptr<ms::Surface> decoration_surface;
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<ms::Session> session;
    std::shared_ptr<md::WindowState> window_state;

    Renderer::RenderingCallback const render_titlebar;
    Renderer::RenderingCallback const render_left_border;
    Renderer::RenderingCallback const render_right_border;
    Renderer::RenderingCallback const render_bottom_border;

    OnProcessEnter const on_process_enter;
    OnProcessLeave const on_process_leave;
    OnProcessDown const on_process_down;
    OnProcessUp const on_process_up;
    OnProcessMove const on_process_move;
    OnProcessDrag const on_process_drag;

    OnWindowStateUpdated on_update_decoration_window_state;
    OnWindowAttribChanged on_attrib_changed;
    OnWindowResized on_window_resized_to;
    OnWindowRenamed on_window_renamed;

    std::shared_ptr<md::StaticGeometry> geometry;
};

miral::DecorationAdapter::DecorationAdapter(
    std::shared_ptr<DecorationRedrawNotifier> const& redraw_notifier,
    Renderer::RenderingCallback render_titlebar,
    Renderer::RenderingCallback render_left_border,
    Renderer::RenderingCallback render_right_border,
    Renderer::RenderingCallback render_bottom_border,
    OnProcessEnter process_enter,
    OnProcessLeave process_leave,
    OnProcessDown process_down,
    OnProcessUp process_up,
    OnProcessMove process_move,
    OnProcessDrag process_drag,
    OnWindowAttribChanged attrib_changed,
    OnWindowResized window_resized_to,
    OnWindowRenamed window_renamed,
    OnWindowStateUpdated update_decoration_window_state) :
    impl{std::make_shared<Impl>(
        redraw_notifier,
        render_titlebar,
        render_left_border,
        render_right_border,
        render_bottom_border,
        process_enter,
        process_leave,
        process_down,
        process_up,
        process_move,
        process_drag,
        attrib_changed,
        window_resized_to,
        window_renamed,
        update_decoration_window_state)}
{
}

auto miral::DecorationAdapter::to_decoration() const -> std::shared_ptr<mir::shell::decoration::Decoration>
{
    return impl;
}

void miral::DecorationAdapter::init(
    std::shared_ptr<ms::Surface> const& window_surface,
    std::shared_ptr<ms::Surface> const& decoration_surface,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<mir::input::CursorImages> const& cursor_images)
{
    impl->init(window_surface, decoration_surface, buffer_allocator, shell, cursor_images);
}

void miral::DecorationAdapter::update()
{
    impl->update();
}

auto miral::DecorationAdapter::redraw_notifier() const -> std::shared_ptr<DecorationRedrawNotifier>
{
   return impl->redraw_notifier();
}

void miral::DecorationAdapter::Impl::update()
{
    auto window_spec = [this]
    {
        msh::SurfaceSpecification spec;

        window_surface->set_window_margins(
            as_delta(window_state->titlebar_height()), geom::DeltaX{}, geom::DeltaY{}, geom::DeltaX{});

        if (window_state->window_size().width.as_value())
            spec.width = window_state->window_size().width;
        if (window_state->window_size().height.as_value())
            spec.height = window_state->window_size().height;

        // Could probably be `decoration->get_input_shape(window_state)`.
        // This will do for now.
        spec.input_shape = {
            window_state->titlebar_rect(),
            window_state->bottom_border_rect(),
            window_state->left_border_rect(),
            window_state->right_border_rect(),
        };

        return spec;
    }();

    window_surface->set_window_margins(
        as_delta(window_state->titlebar_height()),
        as_delta(window_state->side_border_width()),
        as_delta(window_state->bottom_border_height()),
        as_delta(window_state->side_border_width()));

    auto stream_spec = renderer->streams_to_spec(window_state);
    stream_spec.update_from(window_spec);

    if (!stream_spec.is_empty())
    {
        shell->modify_surface(session, decoration_surface, stream_spec);
    }

    renderer->update_render_submit(window_state);
}

void miral::DecorationAdapter::set_custom_geometry(std::shared_ptr<md::StaticGeometry> geometry)
{
    impl->set_custom_geometry(geometry);
}

void miral::DecorationAdapter::Impl::set_custom_geometry(std::shared_ptr<md::StaticGeometry> geometry)
{
    this->geometry = geometry;
}

void miral::DecorationAdapter::Impl::init(
    std::shared_ptr<ms::Surface> window_surface,
    std::shared_ptr<ms::Surface> decoration_surface,
    std::shared_ptr<mg::GraphicBufferAllocator> buffer_allocator,
    std::shared_ptr<msh::Shell> shell,
    std::shared_ptr<mir::input::CursorImages> const cursor_images)
{
    this->window_surface = window_surface;
    this->shell = shell;
    this->session = window_surface->session().lock();
    this->decoration_surface = decoration_surface;
    this->window_state = std::make_shared<md::WindowState>(geometry, window_surface.get());

    renderer = std::make_unique<Renderer>(window_surface, buffer_allocator, render_titlebar, render_left_border, render_right_border, render_bottom_border);
    input_adapter = std::make_unique<InputResolverAdapter>(
        decoration_surface,
        shell,
        session,
        window_surface,
        cursor_images,
        on_process_enter,
        on_process_leave,
        on_process_down,
        on_process_up,
        on_process_move,
        on_process_drag);

    window_surface_observer =
        std::make_shared<WindowSurfaceObserver>(on_attrib_changed, on_window_resized_to, on_window_renamed);
    window_surface->register_interest(window_surface_observer);

    // Window state initialize, notify/callback into the user decorations to
    // set themselves up
    on_update_decoration_window_state(window_state);
}

void miral::DecorationAdapter::Impl::window_state_updated(std::shared_ptr<ms::Surface> const window_surface)
{
    window_state = std::make_shared<md::WindowState>(geometry, window_surface.get());
}

miral::DecorationAdapter::DecorationAdapter::Impl::~Impl()
{
    window_surface->unregister_interest(*window_surface_observer);
}
