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

#include "basic_decoration.h"
#include "mir/graphics/buffer.h"
#include "window.h"
#include "input.h"
#include "renderer.h"
#include "threadsafe_access.h"

#include "mir/executor.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/input/cursor_images.h"
#include "mir/wayland/weak.h"

#include <boost/throw_exception.hpp>
#include <functional>
#include <optional>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

namespace mir::shell::decoration
{
// See src/server/shell/decoration/window.h for a full description of each property
StaticGeometry const default_geometry {
    geom::Height{24},   // titlebar_height
    geom::Width{6},     // side_border_width
    geom::Height{6},    // bottom_border_height
    geom::Size{16, 16}, // resize_corner_input_size
    geom::Width{24},    // button_width
    geom::Width{6},     // padding_between_buttons
    geom::Height{14},   // title_font_height
    geom::Point{8, 2},  // title_font_top_left
    geom::Displacement{5, 5}, // icon_padding
    geom::Width{1},     // detail_line_width
};
}

namespace
{
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
    return stream;
}

msd::BasicDecoration::BasicDecoration(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<input::CursorImages> const& cursor_images,
    std::shared_ptr<ms::Surface> const& window_surface)
    : threadsafe_self{std::make_shared<ThreadsafeAccess<BasicDecoration>>(executor)},
      static_geometry{std::make_shared<StaticGeometry>(default_geometry)},
      shell{shell},
      buffer_allocator{buffer_allocator},
      cursor_images{cursor_images},
      session{window_surface->session().lock()},
      buffer_streams{std::make_unique<BufferStreams>(session)},
      renderer{std::make_unique<Renderer>(buffer_allocator, static_geometry)},
      window_surface{window_surface},
      decoration_surface{create_surface()},
      window_state{new_window_state()},
      window_surface_observer_manager{std::make_unique<WindowSurfaceObserverManager>(
          window_surface,
          threadsafe_self)},
      input_manager{std::make_unique<InputManager>(
          static_geometry,
          decoration_surface,
          *window_state,
          threadsafe_self)},
      input_state{input_manager->state()}
{
    if (!session)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("BasicDecoration's window surface has no session"));
    }

    // Trigger a full refresh
    update(std::nullopt, std::nullopt);

    // Calls from the executor thread can come in at any point after this
    threadsafe_self->initialize(this);
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
    auto previous_window_state = std::move(window_state);
    window_state = new_window_state();

    input_manager->update_window_state(*window_state);

    auto const previous_input_state = std::move(input_state);
    input_state = input_manager->state();

    update(previous_window_state.get(), previous_input_state.get());
}

void msd::BasicDecoration::input_state_updated()
{
    // window_state does not need to be updated

    auto const previous_input_state = std::move(input_state);
    input_state = input_manager->state();

    update(window_state.get(), previous_input_state.get());
}

void msd::BasicDecoration::request_move(MirInputEvent const* event)
{
    shell->request_move(session, window_surface, event);
}

void msd::BasicDecoration::request_resize(MirInputEvent const* event, MirResizeEdge edge)
{
    shell->request_resize(session, window_surface, event, edge);
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

void msd::BasicDecoration::set_scale(float new_scale)
{
    scale = new_scale;
    window_state_updated();
}

auto msd::BasicDecoration::new_window_state() const -> std::unique_ptr<WindowState>
{
    return std::make_unique<WindowState>(static_geometry, window_surface, scale);
}

auto msd::BasicDecoration::create_surface() const -> std::shared_ptr<scene::Surface>
{
    msh::SurfaceSpecification params;
    params.type = mir_window_type_decoration;
    params.parent = window_surface;
    auto const size = window_surface->window_size();
    params.width = size.width;
    params.height = size.height;
    params.aux_rect = {{}, {}};
    params.aux_rect_placement_gravity = mir_placement_gravity_northwest;
    params.surface_placement_gravity = mir_placement_gravity_northwest;
    params.placement_hints = MirPlacementHints(0);
    // Will be replaced by initial update
    params.streams = {{
        session->create_buffer_stream(mg::BufferProperties{
            geom::Size{1, 1},
            buffer_format,
            mg::BufferUsage::software}),
        {},
        }};
    return shell->create_surface(session, {}, params, nullptr, nullptr);
}

void msd::BasicDecoration::update(
    std::optional<WindowState const*> previous_window_state,
    std::optional<InputState const*> previous_input_state)
{
    ObjUpdated<WindowState> window_updated{previous_window_state.value_or(nullptr), window_state.get()};
    ObjUpdated<InputState> input_updated{previous_input_state.value_or(nullptr), input_state.get()};

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
                    spec.streams.value().emplace_back(StreamSpecification{stream, as_displacement(rect.top_left)});
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

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::window_name,
            &WindowState::titlebar_rect,
            &WindowState::left_border_rect,
            &WindowState::right_border_rect,
            &WindowState::bottom_border_rect,
            &WindowState::scale}) ||
        input_updated({
            &InputState::buttons}))
    {
        renderer->update_state(*window_state, *input_state);
    }

    std::vector<std::pair<
        std::shared_ptr<mc::BufferStream>,
        std::optional<std::shared_ptr<mg::Buffer>>>> new_buffers;

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::side_border_width,
            &WindowState::side_border_height,
            &WindowState::scale}))
    {
        new_buffers.emplace_back(
            buffer_streams->left_border,
            renderer->render_left_border());
        new_buffers.emplace_back(
            buffer_streams->right_border,
            renderer->render_right_border());
    }

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::bottom_border_width,
            &WindowState::bottom_border_height,
            &WindowState::scale}))
    {
        new_buffers.emplace_back(
            buffer_streams->bottom_border,
            renderer->render_bottom_border());
    }

    if (window_updated({
            &WindowState::focused_state,
            &WindowState::window_name,
            &WindowState::titlebar_rect,
            &WindowState::scale}) ||
        input_updated({
            &InputState::buttons}))
    {
        new_buffers.emplace_back(
            buffer_streams->titlebar,
            renderer->render_titlebar());
    }

    float inv_scale = 1.0f / window_state->scale();
    for (auto const& pair : new_buffers)
    {
        if (pair.second)
            pair.first->submit_buffer(
                pair.second.value(),
                pair.second.value()->size() * inv_scale,
                {{0, 0}, geom::SizeD{pair.second.value()->size()}});
    }
}
