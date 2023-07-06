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

#include "default_window_manager.h"
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

miral::DefaultWindowManager::DefaultWindowManager(
        msh::FocusController* focus_controller,
        std::shared_ptr<msh::DisplayLayout> const& display_layout,
        std::shared_ptr<ms::SessionCoordinator> const& session_coordinator) :
        focus_controller{focus_controller},
        display_layout{display_layout},
        session_coordinator{session_coordinator}
{
}

void miral::DefaultWindowManager::add_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_added(session);
}

void miral::DefaultWindowManager::remove_session(std::shared_ptr<ms::Session> const& session)
{
    on_session_removed(session);
}

auto miral::DefaultWindowManager::add_surface(
        std::shared_ptr<ms::Session> const& session,
        msh::SurfaceSpecification const& params,
        std::function<std::shared_ptr<ms::Surface>(
                std::shared_ptr<ms::Session> const& session,
                msh::SurfaceSpecification const& params)> const& build)
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

void miral::DefaultWindowManager::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    if (auto const session = surface->session().lock())
    {
        on_session_ready(session);
    }
}

void miral::DefaultWindowManager::modify_surface(
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

void miral::DefaultWindowManager::remove_surface(
        std::shared_ptr<ms::Session> const& session,
        std::weak_ptr<ms::Surface> const& surface)
{
    if (auto const locked = surface.lock())
        session->destroy_surface(locked);

    std::lock_guard lock{mutex};
    output_map.erase(surface);
}

void miral::DefaultWindowManager::add_display(mir::geometry::Rectangle const& /*area*/)
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

void miral::DefaultWindowManager::remove_display(mir::geometry::Rectangle const& /*area*/)
{
}

bool miral::DefaultWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
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

bool miral::DefaultWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool miral::DefaultWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

int miral::DefaultWindowManager::set_surface_attribute(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<ms::Surface> const& surface,
        MirWindowAttrib attrib,
        int value)
{
    return surface->configure(attrib, value);
}

void miral::DefaultWindowManager::on_session_added(std::shared_ptr<mir::scene::Session> const& /*session*/) const
{
}

void miral::DefaultWindowManager::on_session_removed(std::shared_ptr<mir::scene::Session> const& session) const
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

void miral::DefaultWindowManager::on_session_ready(std::shared_ptr<mir::scene::Session> const& session) const
{
    focus_controller->set_focus_to(session, session->default_surface());
}

void miral::DefaultWindowManager::handle_raise_surface(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<ms::Surface> const& /*surface*/,
        uint64_t /*timestamp*/)
{
}

void miral::DefaultWindowManager::handle_request_move(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<ms::Surface> const& /*surface*/,
        MirInputEvent const* /*event*/)
{
}

void miral::DefaultWindowManager::handle_request_resize(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<ms::Surface> const& /*surface*/,
        MirInputEvent const* /*event*/,
        MirResizeEdge /*edge*/)
{
}

void miral::DefaultWindowManager::advise_output_create(miral::Output const& output)
{
    std::cout << &output << std::endl;
//    Locker lock{this};
//
//    outputs.add(output.extents());
//
//    add_output_to_display_areas(lock, output);
//    application_zones_need_update = true;
//    policy->advise_output_create(output);
}

void miral::DefaultWindowManager::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    std::cout << &updated << ", " << &original << std::endl;
//    Locker lock{this};
//
//    outputs.remove(original.extents());
//    outputs.add(updated.extents());
//
//    if (original.logical_group_id() == updated.logical_group_id())
//    {
//        for (auto& area : display_areas)
//        {
//            bool update_extents{false};
//            for (auto& output : area->contained_outputs)
//            {
//                if (output.is_same_output(original))
//                {
//                    output = updated;
//                    update_extents = true;
//                }
//            }
//            if (update_extents)
//            {
//                area->area = area->bounding_rectangle_of_contained_outputs();
//            }
//        }
//    }
//    else
//    {
//        remove_output_from_display_areas(lock, original);
//        add_output_to_display_areas(lock, updated);
//    }
//
//    application_zones_need_update = true;
//    policy->advise_output_update(updated, original);
}

void miral::DefaultWindowManager::advise_output_delete(miral::Output const& output)
{
    std::cout << &output << std::endl;
//    Locker lock{this};
//
//    outputs.remove(output.extents());
//
//    remove_output_from_display_areas(lock, output);
//    application_zones_need_update = true;
//    policy->advise_output_delete(output);
}

void miral::DefaultWindowManager::advise_output_end()
{
//    Locker lock{this};
//
//    if (application_zones_need_update)
//    {
//        update_application_zones_and_attached_windows();
//        application_zones_need_update = false;
//    }
}