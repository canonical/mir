/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/system_compositor_window_manager.h"

#include "mir/shell/display_layout.h"
#include "mir/shell/focus_controller.h"
#include "mir/shell/surface_ready_observer.h"
#include "mir/shell/surface_specification.h"

#include "mir/scene/session.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

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
    ms::SurfaceCreationParameters const& params,
    std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build)
-> mf::SurfaceId
{
    mir::geometry::Rectangle rect{params.top_left, params.size};

    if (!params.output_id.as_value())
        BOOST_THROW_EXCEPTION(std::runtime_error("An output ID must be specified"));

    display_layout->place_in_output(params.output_id, rect);

    auto placed_parameters = params;
    placed_parameters.top_left = rect.top_left;
    placed_parameters.size = rect.size;

    auto const result = build(session, placed_parameters);
    auto const surface = session->surface(result);

    auto const session_ready_observer = std::make_shared<SurfaceReadyObserver>(
        [this](std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& /*surface*/)
            {
                on_session_ready(session);
            },
        session,
        surface);

    surface->add_observer(session_ready_observer);
    
    std::lock_guard<decltype(mutex)> lock{mutex};
    output_map[surface] = params.output_id;

    return result;
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

        mir::geometry::Rectangle rect{surface->top_left(), surface->size()};

        if (display_layout->place_in_output(output_id, rect))
        {
            surface->move_to(rect.top_left);
            surface->resize(rect.size);
        }

        std::lock_guard<decltype(mutex)> lock{mutex};
        output_map[surface] = output_id;
    }
}

void msh::SystemCompositorWindowManager::remove_surface(
    std::shared_ptr<ms::Session> const& session,
    std::weak_ptr<ms::Surface> const& surface)
{
    session->destroy_surface(surface);

    std::lock_guard<decltype(mutex)> lock{mutex};
    output_map.erase(surface);
}

void msh::SystemCompositorWindowManager::add_display(mir::geometry::Rectangle const& /*area*/)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    for (auto const& so : output_map)
    {
        if (auto surface = so.first.lock())
        {
            auto const output_id = so.second;

            mir::geometry::Rectangle rect{surface->top_left(), surface->size()};

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

int msh::SystemCompositorWindowManager::set_surface_attribute(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface,
    MirWindowAttrib attrib,
    int value)
{
    return surface->configure(attrib, value);
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

void msh::SystemCompositorWindowManager::handle_request_drag_and_drop(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    uint64_t /*timestamp*/)
{
}

void msh::SystemCompositorWindowManager::handle_request_move(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& /*surface*/,
    uint64_t /*timestamp*/)
{
}

void msh::SystemCompositorWindowManager::handle_request_resize(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& /*surface*/,
    uint64_t /*timestamp*/,
    MirResizeEdge /*edge*/)
{
}
