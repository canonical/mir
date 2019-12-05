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

#include "basic_decoration.h"
#include "window.h"
#include "input.h"
#include "renderer.h"
#include "threadsafe_access.h"

#include "mir/executor.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/input/cursor_images.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <functional>
#include <mutex>
#include <experimental/optional>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

namespace
{
msd::StaticGeometry const default_geometry{
    /*    titlebar_height */ geom::Height{24},
    /*       border_width */ geom::Width{6},
    /*      border_height */ geom::Height{6},
    /*       button_width */ geom::Width{24},
    /* resize_corner_size */ geom::Size{16, 16},
};

template<typename OBJ>
struct PropertyComparison
{
    template<typename RET>
    PropertyComparison(RET(OBJ::*getter)() const)
        : comp{[=](OBJ const* a, OBJ const* b)
              {
                  return (a->*getter)() == (b->*getter)();
              }}
    {
    }

    auto operator()(OBJ const* a, OBJ const* b) const -> bool
    {
        return comp(a, b);
    }

    std::function<bool(OBJ const* a, OBJ const* b)> const comp;
};

template<typename OBJ>
struct ObjUpdated
{
    ObjUpdated(OBJ const* old, OBJ const* current)
        : old{old},
          current{current}
    {
    }

    auto operator()(std::initializer_list<PropertyComparison<OBJ>> comparisons) -> bool
    {
        if (!old)
            return true;
        for (auto const& comparison : comparisons)
            if (!comparison(old, current))
                return true;
        return false;
    }

    OBJ const* const old;
    OBJ const* const current;
};
}

class msd::BasicDecoration::BufferStreams
{
    // Must be at top so it can be used by create_buffer_stream() when called in the constructor
    std::shared_ptr<scene::Session> const session;

public:
    BufferStreams(std::shared_ptr<scene::Session> const& session);
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

msd::BasicDecoration::BufferStreams::BufferStreams(std::shared_ptr<scene::Session> const& session)
    : session{session},
      titlebar{create_buffer_stream()},
      left_border{create_buffer_stream()},
      right_border{create_buffer_stream()},
      bottom_border{create_buffer_stream()}
{
}

msd::BasicDecoration::BufferStreams::~BufferStreams()
{
    session->destroy_buffer_stream(titlebar);
    session->destroy_buffer_stream(left_border);
    session->destroy_buffer_stream(right_border);
    session->destroy_buffer_stream(bottom_border);
}

auto msd::BasicDecoration::BufferStreams::create_buffer_stream() -> std::shared_ptr<mc::BufferStream>
{
    auto const stream = session->create_buffer_stream(mg::BufferProperties{
        geom::Size{1, 1},
        buffer_format,
        mg::BufferUsage::software});
    stream->allow_framedropping(true);
    return stream;
}

msd::BasicDecoration::BasicDecoration(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<input::CursorImages> const& cursor_images,
    std::shared_ptr<ms::Surface> const& window_surface)
    : threadsafe_self{std::make_shared<ThreadsafeAccess<BasicDecoration>>(this, executor)},
      static_geometry{std::make_unique<StaticGeometry>(default_geometry)},
      window_state{nullptr},
      input_state{nullptr},
      shell{shell},
      buffer_allocator{buffer_allocator},
      cursor_images{cursor_images},
      session{[&]() -> std::shared_ptr<ms::Session>
          {
              auto const session = window_surface->session().lock();
              if (!session)
              {
                  threadsafe_self->invalidate();
                  BOOST_THROW_EXCEPTION(std::runtime_error("BasicDecoration's window surface has no session"));
              }
              return session;
          }()},
      buffer_streams{std::make_unique<BufferStreams>(session)},
      renderer{std::make_unique<Renderer>(buffer_allocator)},
      window_surface{window_surface},
      decoration_surface{shell->create_surface(session, *creation_params(), nullptr)},
      window_surface_observer_manager{std::make_unique<WindowSurfaceObserverManager>(
          window_surface,
          threadsafe_self)},
      input_manager{std::make_unique<InputManager>(
          decoration_surface,
          *new_window_state(),
          threadsafe_self)}
{
    try
    {
        update();
    }
    catch (...)
    {
        threadsafe_self->invalidate();
        throw;
    }
}

msd::BasicDecoration::~BasicDecoration()
{
    threadsafe_self->invalidate();
    shell->destroy_surface(session, decoration_surface);
    window_surface->set_window_margins(
        geom::DeltaY{},
        geom::DeltaX{},
        geom::DeltaY{},
        geom::DeltaX{});
}

void msd::BasicDecoration::window_state_updated()
{
    input_manager->update_window_state(*new_window_state());
    update();
}

void msd::BasicDecoration::input_state_updated()
{
    update();
}

void msd::BasicDecoration::request_move(std::chrono::nanoseconds const& timestamp)
{
    shell->request_move(session, window_surface, timestamp.count());
}

void msd::BasicDecoration::request_resize(std::chrono::nanoseconds const& timestamp, MirResizeEdge edge)
{
    shell->request_resize(session, window_surface, timestamp.count(), edge);
}

void msd::BasicDecoration::request_toggle_maximize()
{
    msh::SurfaceSpecification spec;
    if (window_surface->state() == mir_window_state_maximized)
        spec.state = mir_window_state_restored;
    else
        spec.state = mir_window_state_maximized;
    shell->modify_surface(session, window_surface, spec);
}

void msd::BasicDecoration::request_minimize()
{
    msh::SurfaceSpecification spec;
    spec.state = mir_window_state_minimized;
    shell->modify_surface(session, window_surface, spec);
}

void msd::BasicDecoration::request_close()
{
    window_surface->request_client_surface_close();
}

void msd::BasicDecoration::set_cursor(std::string const& cursor_image_name)
{
    msh::SurfaceSpecification spec;
    // size is hard-coded because current implementation ignores it
    spec.cursor_image = cursor_images->image(cursor_image_name, {16, 16});
    shell->modify_surface(session, decoration_surface, spec);
}

auto msd::BasicDecoration::new_window_state() const -> std::unique_ptr<WindowState>
{
    return std::make_unique<WindowState>(*static_geometry, window_surface);
}

auto msd::BasicDecoration::creation_params() const -> std::unique_ptr<scene::SurfaceCreationParameters>
{
    auto params = std::make_unique<ms::SurfaceCreationParameters>();
    params->type = mir_window_type_decoration;
    params->parent = window_surface;
    params->size = window_surface->window_size();
    params->aux_rect = {{}, {}};
    params->aux_rect_placement_gravity = mir_placement_gravity_northwest;
    params->surface_placement_gravity = mir_placement_gravity_northwest;
    params->placement_hints = MirPlacementHints(0);
    // Will be replaced by initial update
    params->content = session->create_buffer_stream(mg::BufferProperties{
        geom::Size{1, 1},
        buffer_format,
        mg::BufferUsage::software});
    return params;
}

void msd::BasicDecoration::update()
{
    auto const old_window_state = std::move(window_state);
    window_state = new_window_state();
    ObjUpdated<WindowState> window_updated{old_window_state.get(), window_state.get()};

    auto const old_input_state = std::move(input_state);
    input_state = input_manager->state();
    ObjUpdated<InputState> input_updated{old_input_state.get(), input_state.get()};

    if (window_updated({
            &WindowState::titlebar_height,
            &WindowState::side_border_width,
            &WindowState::bottom_border_height}))
    {
        window_surface->set_window_margins(
            as_delta(window_state->titlebar_height()),
            as_delta(window_state->side_border_width()),
            as_delta(window_state->bottom_border_height()),
            as_delta(window_state->side_border_width()));
    }

    msh::SurfaceSpecification spec;

    if (window_updated({
            &WindowState::window_size}))
    {
        spec.width = window_state->window_size().width;
        spec.height = window_state->window_size().height;
    }

    if (input_updated({
            &InputState::input_shape}))
    {
        spec.input_shape = input_state->input_shape();
    }

    if (window_updated({
            &WindowState::border_type,
            &WindowState::titlebar_rect,
            &WindowState::left_border_rect,
            &WindowState::right_border_rect,
            &WindowState::bottom_border_rect}))
    {
        spec.streams = std::vector<StreamSpecification>{};
        auto const emplace = [&](std::shared_ptr<mc::BufferStream> stream, geom::Rectangle rect)
            {
                if (rect.size.width > geom::Width{} && rect.size.height > geom::Height{})
                    spec.streams.value().emplace_back(StreamSpecification{stream, as_displacement(rect.top_left), rect.size});
            };

        switch (window_state->border_type())
        {
        case BorderType::Full:
            emplace(buffer_streams->titlebar, window_state->titlebar_rect());
            emplace(buffer_streams->left_border, window_state->left_border_rect());
            emplace(buffer_streams->right_border, window_state->right_border_rect());
            emplace(buffer_streams->bottom_border, window_state->bottom_border_rect());
            break;
        case BorderType::Titlebar:
            emplace(buffer_streams->titlebar, window_state->titlebar_rect());
            break;
        case BorderType::None:
            break;
        };
    }

    if (!spec.is_empty())
    {
        shell->modify_surface(session, decoration_surface, spec);
    }

    std::vector<std::pair<
        std::shared_ptr<mc::BufferStream>,
        std::experimental::optional<std::shared_ptr<mg::Buffer>>>> new_buffers;

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::side_border_width,
            &WindowState::side_border_height}))
    {
        new_buffers.emplace_back(
            buffer_streams->left_border,
            renderer->render_left_border(*window_state));
        new_buffers.emplace_back(
            buffer_streams->right_border,
            renderer->render_right_border(*window_state));
    }

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::bottom_border_width,
            &WindowState::bottom_border_height}))
    {
        new_buffers.emplace_back(
            buffer_streams->bottom_border,
            renderer->render_bottom_border(*window_state));
    }

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::window_name,
            &WindowState::titlebar_rect}) ||
        input_updated({
            &InputState::buttons}))
    {
        new_buffers.emplace_back(
            buffer_streams->titlebar,
            renderer->render_titlebar(*window_state, *input_state));
    }

    for (auto const& pair : new_buffers)
    {
        if (pair.second)
            pair.first->submit_buffer(pair.second.value());
    }
}
