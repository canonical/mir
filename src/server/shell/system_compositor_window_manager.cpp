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

#include "mir/shell/system_compositor_window_manager.h"

#include "mir/shell/display_layout.h"
#include "mir/shell/focus_controller.h"
#include "mir/shell/surface_specification.h"

#include "mir/scene/session.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/surface.h"

#include <boost/throw_exception.hpp>

#include <linux/input.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

msh::SystemCompositorWindowManager::SystemCompositorWindowManager(
    FocusController* focus_controller,
    std::shared_ptr<DisplayLayout> const& display_layout,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator) :
    focus_controller{focus_controller},
    display_layout{display_layout},
    session_coordinator{session_coordinator}
{
}

void msh::SystemCompositorWindowManager::add_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_added(session);
}

void msh::SystemCompositorWindowManager::remove_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_removed(session);
}

auto msh::SystemCompositorWindowManager::add_surface(
    std::shared_ptr<ms::Session> const& session,
    shell::SurfaceSpecification const& params,
    std::function<std::shared_ptr<scene::Surface>(
        std::shared_ptr<ms::Session> const& session,
        shell::SurfaceSpecification const& params)> const& build)
-> std::shared_ptr<scene::Surface>
{
    mir::geometry::Rectangle rect;
    if (params.top_left.is_set())
    {
        rect.top_left = params.top_left.value();
    }
    if (params.width.is_set())
    {
        rect.size.width = params.width.value();
    }
    if (params.height.is_set())
    {
        rect.size.height = params.height.value();
    }

    if (!params.output_id.is_set() || !params.output_id.value().as_value())
        BOOST_THROW_EXCEPTION(std::runtime_error("An output ID must be specified"));

    display_layout->place_in_output(params.output_id.value(), rect);

    auto placed_parameters = params;
    placed_parameters.top_left = rect.top_left;
    placed_parameters.width = rect.size.width;
    placed_parameters.height = rect.size.height;

    auto const surface = build(session, placed_parameters);

    std::lock_guard lock{mutex};
    output_map[surface] = params.output_id.value();

    return surface;
}

void msh::SystemCompositorWindowManager::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    if (auto const session = surface->session().lock())
    {
        on_session_ready(session);
    }
}

void msh::SystemCompositorWindowManager::modify_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface,
    SurfaceSpecification const& modifications)
{
    if (modifications.name.is_set())
        surface->rename(modifications.name.value());

    if (modifications.output_id.is_set())
    {
        auto const output_id = modifications.output_id.value();

        mir::geometry::Rectangle rect{surface->top_left(), surface->window_size()};

        if (display_layout->place_in_output(output_id, rect))
        {
            surface->move_to(rect.top_left);
            surface->resize(rect.size);
        }

        std::lock_guard lock{mutex};
        output_map[surface] = output_id;
    }

    if (modifications.input_shape.is_set())
    {
        surface->set_input_region(modifications.input_shape.value());
    }
}

void msh::SystemCompositorWindowManager::remove_surface(
    std::shared_ptr<ms::Session> const& session,
    std::weak_ptr<ms::Surface> const& surface)
{
    if (auto const locked = surface.lock())
        session->destroy_surface(locked);

    std::lock_guard lock{mutex};
    output_map.erase(surface);
}

void msh::SystemCompositorWindowManager::add_display(mir::geometry::Rectangle const& /*area*/)
{
    std::lock_guard lock{mutex};

    for (auto const& so : output_map)
    {
        if (auto surface = so.first.lock())
        {
            auto const output_id = so.second;

            mir::geometry::Rectangle rect{surface->top_left(), surface->window_size()};

            if (display_layout->place_in_output(output_id, rect))
            {
                surface->move_to(rect.top_left);
                surface->resize(rect.size);
            }
        }
    }
}

void msh::SystemCompositorWindowManager::remove_display(mir::geometry::Rectangle const& /*area*/)
{
}

bool msh::SystemCompositorWindowManager::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool msh::SystemCompositorWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool msh::SystemCompositorWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

void msh::SystemCompositorWindowManager::on_session_added(std::shared_ptr<mir::scene::Session> const& /*session*/) const
{
}

void msh::SystemCompositorWindowManager::on_session_removed(std::shared_ptr<mir::scene::Session> const& session) const
{
    if (focus_controller->focused_session() == session)
    {
        auto const next_session = session_coordinator->successor_of({});
        if (next_session)
            focus_controller->set_focus_to(next_session, next_session->default_surface());
        else
            focus_controller->set_focus_to(next_session, {});
    }
}

void msh::SystemCompositorWindowManager::on_session_ready(std::shared_ptr<mir::scene::Session> const& session) const
{
    focus_controller->set_focus_to(session, session->default_surface());
}

void msh::SystemCompositorWindowManager::handle_raise_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    uint64_t /*timestamp*/)
{
}

void msh::SystemCompositorWindowManager::handle_request_move(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    MirInputEvent const* /*event*/)
{
}

void msh::SystemCompositorWindowManager::handle_request_resize(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& /*surface*/,
    MirInputEvent const* /*event*/,
    MirResizeEdge /*edge*/)
{
}

bool mir::shell::DefaultWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    static unsigned int const shift_states =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    if (mir_keyboard_event_action(event) != mir_keyboard_action_down)
        return false;

    auto const shift_state = mir_keyboard_event_modifiers(event) & shift_states;

    if (shift_state != (mir_input_event_modifier_alt|mir_input_event_modifier_ctrl))
        return false;

    switch (mir_keyboard_event_scan_code(event))
    {
    case KEY_PAGEDOWN:
        focus_controller->focus_next_session();
        break;

    case KEY_PAGEUP:
        focus_controller->focus_prev_session();
        break;

    default:
        return false;
    };

    if (auto const session = focus_controller->focused_session())
    {
        SurfaceSet surfaces;
        for (auto surface = session->default_surface(); surface; surface = session->surface_after(surface))
        {
            if (!surfaces.insert(surface).second) break;
        }
        focus_controller->raise(surfaces);
        focus_controller->set_focus_to(session, session->default_surface());
    }
    return true;
}
