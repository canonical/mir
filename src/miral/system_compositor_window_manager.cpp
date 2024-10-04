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

#include "system_compositor_window_manager.h"
#include "display_configuration_listeners.h"
#include <miral/output.h>

#include <mir/shell/display_layout.h>
#include <mir/shell/focus_controller.h>
#include <mir/shell/surface_specification.h>

#include <mir/scene/session.h>
#include <mir/scene/session_coordinator.h>
#include <mir/scene/surface.h>


#include <boost/throw_exception.hpp>

#include <linux/input.h>
#include <iostream>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

miral::SystemCompositorWindowManager::SystemCompositorWindowManager(
    msh::FocusController* focus_controller,
    std::shared_ptr<msh::DisplayLayout> const& display_layout,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers) :
    focus_controller{focus_controller},
    display_layout{display_layout},
    session_coordinator{session_coordinator},
    display_config_monitor{std::make_shared<DisplayConfigurationListeners>()}
{
    display_config_monitor->add_listener(this);
    display_configuration_observers.register_interest(display_config_monitor);
}

miral::SystemCompositorWindowManager::~SystemCompositorWindowManager() {
    display_config_monitor->delete_listener(this);
}

void miral::SystemCompositorWindowManager::add_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_added(session);
}

void miral::SystemCompositorWindowManager::remove_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_removed(session);
}

auto miral::SystemCompositorWindowManager::add_surface(
    std::shared_ptr<ms::Session> const& session,
    msh::SurfaceSpecification const& params,
    MirSurfaceCreator const& build)
-> std::shared_ptr<ms::Surface>
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

void miral::SystemCompositorWindowManager::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    if (auto const session = surface->session().lock())
    {
        on_session_ready(session);
    }
}

void miral::SystemCompositorWindowManager::modify_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface,
    msh::SurfaceSpecification const& modifications)
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

void miral::SystemCompositorWindowManager::remove_surface(
    std::shared_ptr<ms::Session> const& session,
    std::weak_ptr<ms::Surface> const& surface)
{
    if (auto const locked = surface.lock())
        session->destroy_surface(locked);

    std::lock_guard lock{mutex};
    output_map.erase(surface);
}

void miral::SystemCompositorWindowManager::add_display(mir::geometry::Rectangle const& /*area*/)
{
}

void miral::SystemCompositorWindowManager::remove_display(mir::geometry::Rectangle const& /*area*/)
{
}

bool miral::SystemCompositorWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
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
        msh::SurfaceSet surfaces;
        for (auto surface = session->default_surface(); surface; surface = session->surface_after(surface))
        {
            if (!surfaces.insert(surface).second) break;
        }
        focus_controller->raise(surfaces);
        focus_controller->set_focus_to(session, session->default_surface());
    }
    return true;
}

bool miral::SystemCompositorWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool miral::SystemCompositorWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

void miral::SystemCompositorWindowManager::on_session_added(std::shared_ptr<mir::scene::Session> const& /*session*/) const
{
}

void miral::SystemCompositorWindowManager::on_session_removed(std::shared_ptr<mir::scene::Session> const& session) const
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

void miral::SystemCompositorWindowManager::on_session_ready(std::shared_ptr<mir::scene::Session> const& session) const
{
    focus_controller->set_focus_to(session, session->default_surface());
}

void miral::SystemCompositorWindowManager::handle_raise_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    uint64_t /*timestamp*/)
{
}

void miral::SystemCompositorWindowManager::handle_request_move(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    MirInputEvent const* /*event*/)
{
}

void miral::SystemCompositorWindowManager::handle_request_resize(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    MirInputEvent const* /*event*/,
    MirResizeEdge /*edge*/)
{
}

void miral::SystemCompositorWindowManager::advise_output_create(const miral::Output &/*output*/)
{
    // When a display gets added, we reposition all surfaces across their outputs, most likely to fit them to the screen.
    for (auto const& surface_output_pair : output_map)
    {
        if (auto surface = surface_output_pair.first.lock())
        {
            auto const output_id = surface_output_pair.second;
            reposition_surface_in_output(surface, output_id);
        }
    }
}

void miral::SystemCompositorWindowManager::advise_output_update(const miral::Output & updated, const miral::Output &/*original*/)
{
    std::lock_guard lock{mutex};

    // When a display gets updated, we reposition any surface that is on the updated Output.
    for (auto const& surface_output_pair : output_map)
    {
        if (auto surface = surface_output_pair.first.lock())
        {
            auto const output_id = surface_output_pair.second;
            if (updated.id() == output_id.as_value())
            {
                reposition_surface_in_output(surface, output_id);
            }
        }
    }
}

void miral::SystemCompositorWindowManager::advise_output_delete(const miral::Output &/*output*/)
{
}

void miral::SystemCompositorWindowManager::reposition_surface_in_output(
    std::shared_ptr<mir::scene::Surface> const& surface,
    mir::graphics::DisplayConfigurationOutputId output_id
)
{
    mir::geometry::Rectangle surface_rect{surface->top_left(), surface->window_size()};
    if (display_layout->place_in_output(output_id, surface_rect))
    {
        surface->move_to(surface_rect.top_left);
        surface->resize(surface_rect.size);
    }
}