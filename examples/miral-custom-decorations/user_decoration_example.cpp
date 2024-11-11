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

#include "user_decoration_example.h"
#include "window.h"
#include "renderer.h"
#include "threadsafe_access.h"

#include "mir/graphics/buffer.h"
#include "mir/executor.h"
#include "mir/shell/shell.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/input/cursor_images.h"
#include "mirwayland/mir/wayland/weak.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace geom = mir::geometry;


namespace
{

// See window.h for a full description of each property
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

uint32_t const default_focused_background = color(0x32, 0x32, 0x32);
uint32_t const default_focused_text = color(0xFF, 0xFF, 0xFF);
uint32_t const default_unfocused_background = color(0x80, 0x80, 0x80);
uint32_t const default_unfocused_text = color(0xA0, 0xA0, 0xA0);

static Theme const default_focused{default_focused_background, default_focused_text};
static Theme const default_unfocused{default_unfocused_background, default_unfocused_text};

uint32_t const default_normal_button = color(0x60, 0x60, 0x60);
uint32_t const default_active_button = color(0xA0, 0xA0, 0xA0);
uint32_t const default_close_normal_button = color(0xA0, 0x20, 0x20);
uint32_t const default_close_active_button = color(0xC0, 0x60, 0x60);
uint32_t const default_button_icon = color(0xFF, 0xFF, 0xFF);

static ButtonTheme const default_button_themes = {
    {
        ButtonFunction::Close,
        {default_close_normal_button, default_close_active_button, default_button_icon},
    },
    {
        ButtonFunction::Maximize,
        {default_normal_button, default_active_button, default_button_icon},
    },
    {
        ButtonFunction::Minimize,
        {default_normal_button, default_active_button, default_button_icon},
    },
};

template <typename OBJ>
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

class UserDecorationExample::BufferStreams
{
    // Must be at top so it can be used by create_buffer_stream() when called in the constructor
    std::shared_ptr<mir::scene::Session> const session;

public:
    BufferStreams(std::shared_ptr<mir::scene::Session> const& session);
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

UserDecorationExample::BufferStreams::BufferStreams(std::shared_ptr<mir::scene::Session> const& session)
    : session{session},
      titlebar{create_buffer_stream()},
      left_border{create_buffer_stream()},
      right_border{create_buffer_stream()},
      bottom_border{create_buffer_stream()}
{
}

UserDecorationExample::BufferStreams::~BufferStreams()
{
    session->destroy_buffer_stream(titlebar);
    session->destroy_buffer_stream(left_border);
    session->destroy_buffer_stream(right_border);
    session->destroy_buffer_stream(bottom_border);
}

auto UserDecorationExample::BufferStreams::create_buffer_stream() -> std::shared_ptr<mc::BufferStream>
{
    auto const stream = session->create_buffer_stream(mg::BufferProperties{
        geom::Size{1, 1},
        buffer_format,
        mg::BufferUsage::software});
    return stream;
}

auto create_surface(std::shared_ptr<ms::Surface> const& window_surface, std::shared_ptr<msh::Shell> const& shell)
    -> std::shared_ptr<mir::scene::Surface>
{
    auto session = window_surface->session().lock();
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

UserDecorationExample::UserDecorationExample(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mir::Executor> const& executor,
    std::shared_ptr<mir::input::CursorImages> const& cursor_images,
    std::shared_ptr<ms::Surface> const& window_surface) :
    UserDecorationExample(shell, buffer_allocator, executor, cursor_images, window_surface, default_focused, default_unfocused, default_button_themes)
{
}

UserDecorationExample::UserDecorationExample(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mir::Executor> const& executor,
    std::shared_ptr<mir::input::CursorImages> const& cursor_images,
    std::shared_ptr<ms::Surface> const& window_surface,
    Theme focused,
    Theme unfocused,
    ButtonTheme const button_theme) :
    decoration_surface_{create_surface(window_surface, shell)},
    threadsafe_self{std::make_shared<ThreadsafeAccess<UserDecorationExample>>(executor)},
    static_geometry{std::make_shared<StaticGeometry>(default_geometry)},
    shell{shell},
    buffer_allocator{buffer_allocator},
    cursor_images{cursor_images},
    session{window_surface->session().lock()},
    buffer_streams{std::make_unique<BufferStreams>(session)},
    renderer{std::make_unique<Renderer>(buffer_allocator, static_geometry, focused, unfocused, button_theme)},
    window_surface_{window_surface},
    window_state{new_window_state()},
    input_manager{std::make_unique<InputManager>(static_geometry, *window_state, threadsafe_self)},
    input_state{input_manager->state()}
{
    if (!session)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("UserDecorationExample's window surface has no session"));
    }

    // Trigger a full refresh
    update(std::nullopt, std::nullopt);

    // Calls from the executor thread can come in at any point after this
    threadsafe_self->initialize(this);
}

UserDecorationExample::~UserDecorationExample()
{
    threadsafe_self->invalidate();
    shell->destroy_surface(session, decoration_surface_);
    window_surface_->set_window_margins(
        geom::DeltaY{},
        geom::DeltaX{},
        geom::DeltaY{},
        geom::DeltaX{});
}

void UserDecorationExample::redraw()
{
    auto previous_window_state = std::move(window_state);
    window_state = new_window_state();

    input_manager->update_window_state(*window_state);

    auto const previous_input_state = std::move(input_state);
    input_state = input_manager->state();

    update(previous_window_state.get(), previous_input_state.get());
}

void UserDecorationExample::handle_input_event(std::shared_ptr<MirEvent const> const& event)
{
    input_manager->handle_input_event(event);
}

void UserDecorationExample::input_state_changed()
{
    // window_state does not need to be updated

    auto const previous_input_state = std::move(input_state);
    input_state = input_manager->state();

    update(window_state.get(), previous_input_state.get());
}

void UserDecorationExample::request_move(MirInputEvent const* event)
{
    shell->request_move(session, window_surface_, event);
}

void UserDecorationExample::request_resize(MirInputEvent const* event, MirResizeEdge edge)
{
    shell->request_resize(session, window_surface_, event, edge);
}

void UserDecorationExample::request_toggle_maximize()
{
    msh::SurfaceSpecification spec;
    if (window_surface_->state() == mir_window_state_maximized)
        spec.state = mir_window_state_restored;
    else
        spec.state = mir_window_state_maximized;
    shell->modify_surface(session, window_surface_, spec);
}

void UserDecorationExample::request_minimize()
{
    msh::SurfaceSpecification spec;
    spec.state = mir_window_state_minimized;
    shell->modify_surface(session, window_surface_, spec);
}

void UserDecorationExample::request_close()
{
    window_surface_->request_client_surface_close();
}

void UserDecorationExample::set_cursor(std::string const& cursor_image_name)
{
    msh::SurfaceSpecification spec;
    // size is hard-coded because current implementation ignores it
    spec.cursor_image = cursor_images->image(cursor_image_name, {16, 16});
    shell->modify_surface(session, decoration_surface_, spec);
}

std::shared_ptr<mir::scene::Surface> UserDecorationExample::decoration_surface()
{
    return decoration_surface_;
}

std::shared_ptr<mir::scene::Surface> UserDecorationExample::window_surface()
{
    return window_surface_;
}

void UserDecorationExample::set_scale(float new_scale)
{
    threadsafe_self->spawn(
        [new_scale](auto* deco)
        {
            deco->scale = new_scale;
            deco->redraw();
        });
}

void UserDecorationExample::attrib_changed(mir::scene::Surface const*, MirWindowAttrib attrib, int)
{
    switch (attrib)
    {
    case mir_window_attrib_type:
    case mir_window_attrib_state:
    case mir_window_attrib_focus:
    case mir_window_attrib_visibility:
        threadsafe_self->spawn([](auto* decoration) { decoration->redraw(); });
        break;

    case mir_window_attrib_dpi:
    case mir_window_attrib_preferred_orientation:
    case mir_window_attribs:
        break;
    }
}

void UserDecorationExample::window_resized_to(mir::scene::Surface const*, geom::Size const&)
{
    threadsafe_self->spawn(
        [](auto* decoration)
        {
            decoration->redraw();
        });
}

void UserDecorationExample::window_renamed(mir::scene::Surface const*, std::string const&)
{
    threadsafe_self->spawn(
        [](auto* decoration)
        {
            decoration->redraw();
        });
}

auto UserDecorationExample::new_window_state() const -> std::unique_ptr<WindowState>
{
    return std::make_unique<WindowState>(static_geometry, window_surface_, scale);
}

void UserDecorationExample::update(
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
        window_surface_->set_window_margins(
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
        spec.streams = std::vector<msh::StreamSpecification>{};
        auto const emplace = [&](std::shared_ptr<mc::BufferStream> stream, geom::Rectangle rect)
            {
                if (rect.size.width > geom::Width{} && rect.size.height > geom::Height{})
                    spec.streams.value().emplace_back(msh::StreamSpecification{stream, as_displacement(rect.top_left)});
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
        shell->modify_surface(session, decoration_surface_, spec);
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
