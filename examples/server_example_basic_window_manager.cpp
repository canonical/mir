/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_basic_window_manager.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

namespace me = mir::examples;

me::BasicWindowManager::BasicWindowManager(
    shell::FocusController* focus_controller,
    std::unique_ptr<WindowManagementPolicy> policy) :
    focus_controller(focus_controller),
    policy(std::move(policy))
{
}

void me::BasicWindowManager::add_session(std::shared_ptr<scene::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    session_info[session] = SessionInfo();
    policy->handle_session_info_updated(session_info, displays);
}

void me::BasicWindowManager::remove_session(std::shared_ptr<scene::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    session_info.erase(session);
    policy->handle_session_info_updated(session_info, displays);
}

auto me::BasicWindowManager::add_surface(
    std::shared_ptr<scene::Session> const& session,
    scene::SurfaceCreationParameters const& params,
    std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build)
-> frontend::SurfaceId
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    scene::SurfaceCreationParameters const placed_params = policy->handle_place_new_surface(session, params);
    auto const result = build(session, placed_params);
    auto const surface = session->surface(result);
    surface_info.emplace(surface, SurfaceInfo{session, surface, placed_params});
    policy->handle_new_surface(session, surface);
    policy->generate_decorations_for(session, surface);
    return result;
}

void me::BasicWindowManager::modify_surface(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    shell::SurfaceSpecification const& modifications)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    policy->handle_modify_surface(session, surface, modifications);
}

void me::BasicWindowManager::remove_surface(
    std::shared_ptr<scene::Session> const& session,
    std::weak_ptr<scene::Surface> const& surface)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    policy->handle_delete_surface(session, surface);

    surface_info.erase(surface);
}

void me::BasicWindowManager::forget(std::weak_ptr<scene::Surface> const& surface)
{
    surface_info.erase(surface);
}

void me::BasicWindowManager::add_display(geometry::Rectangle const& area)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    displays.add(area);
    policy->handle_displays_updated(session_info, displays);
}

void me::BasicWindowManager::remove_display(geometry::Rectangle const& area)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    displays.remove(area);
    policy->handle_displays_updated(session_info, displays);
}

bool me::BasicWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    update_event_timestamp(event);
    return policy->handle_keyboard_event(event);
}

bool me::BasicWindowManager::handle_touch_event(MirTouchEvent const* event)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    update_event_timestamp(event);
    return policy->handle_touch_event(event);
}

bool me::BasicWindowManager::handle_pointer_event(MirPointerEvent const* event)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    update_event_timestamp(event);

    cursor = {
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    return policy->handle_pointer_event(event);
}

void me::BasicWindowManager::handle_raise_surface(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    uint64_t timestamp)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (timestamp >= last_input_event_timestamp)
        policy->handle_raise_surface(session, surface);
}

int me::BasicWindowManager::set_surface_attribute(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    switch (attrib)
    {
    case mir_surface_attrib_state:
    {
        auto const state = policy->handle_set_state(surface, MirSurfaceState(value));
        return surface->configure(attrib, state);
    }
    default:
        return surface->configure(attrib, value);
    }
}

auto me::BasicWindowManager::find_session(std::function<bool(SessionInfo const& info)> const& predicate)
-> std::shared_ptr<scene::Session>
    {
        for(auto& info : session_info)
        {
            if (predicate(info.second))
            {
                return info.first.lock();
            }
        }

        return std::shared_ptr<scene::Session>{};
    }

auto me::BasicWindowManager::info_for(std::weak_ptr<scene::Session> const& session) const
-> SessionInfo&
{
    return const_cast<SessionInfo&>(session_info.at(session));
}

auto me::BasicWindowManager::info_for(std::weak_ptr<scene::Surface> const& surface) const
-> SurfaceInfo&
{
    return const_cast<SurfaceInfo&>(surface_info.at(surface));
}

auto me::BasicWindowManager::focused_session() const
-> std::shared_ptr<scene::Session>
{
    return focus_controller->focused_session();
}

auto me::BasicWindowManager::focused_surface() const
->std::shared_ptr<scene::Surface>
{
    return focus_controller->focused_surface();
}

void me::BasicWindowManager::focus_next_session()
{
    focus_controller->focus_next_session();
}

void me::BasicWindowManager::set_focus_to(
    std::shared_ptr<scene::Session> const& focus,
    std::shared_ptr<scene::Surface> const& surface)
{
    focus_controller->set_focus_to(focus, surface);
}

auto me::BasicWindowManager::surface_at(geometry::Point cursor) const
-> std::shared_ptr<scene::Surface>
{
    return focus_controller->surface_at(cursor);
}

auto me::BasicWindowManager::active_display()
-> geometry::Rectangle const
{
    geometry::Rectangle result;

    // 1. If a window has input focus, whichever display contains the largest
    //    proportion of the area of that window.
    if (auto const surface = focused_surface())
    {
        auto const surface_rect = surface->input_bounds();
        int max_overlap_area = -1;

        for (auto const& display : displays)
        {
            auto const intersection = surface_rect.intersection_with(display).size;
            if (intersection.width.as_int()*intersection.height.as_int() > max_overlap_area)
            {
                max_overlap_area = intersection.width.as_int()*intersection.height.as_int();
                result = display;
            }
        }
        return result;
    }

    // 2. Otherwise, if any window previously had input focus, for the window that had
    //    it most recently, the display that contained the largest proportion of the
    //    area of that window at the moment it closed, as long as that display is still
    //    available.

    // 3. Otherwise, the display that contains the pointer, if there is one.
    for (auto const& display : displays)
    {
        if (display.contains(cursor))
        {
            // Ignore the (unspecified) possiblity of overlapping displays
            return display;
        }
    }

    // 4. Otherwise, the primary display, if there is one (for example, the laptop display).

    // 5. Otherwise, the first display.
    if (displays.size())
        result = *displays.begin();

    return result;
}

void me::BasicWindowManager::raise_tree(std::shared_ptr<scene::Surface> const& root)
{
    SurfaceSet surfaces;
    std::function<void(std::weak_ptr<scene::Surface> const& surface)> const add_children =
        [&,this](std::weak_ptr<scene::Surface> const& surface)
            {
            auto const& info = info_for(surface);
            surfaces.insert(begin(info.children), end(info.children));
            for (auto const& child : info.children)
                add_children(child);
            };

    surfaces.insert(root);
    add_children(root);

    focus_controller->raise(surfaces);
}

void me::BasicWindowManager::update_event_timestamp(MirKeyboardEvent const* kev)
{
    auto iev = mir_keyboard_event_input_event(kev);
    last_input_event_timestamp = mir_input_event_get_event_time(iev);
}

void me::BasicWindowManager::update_event_timestamp(MirPointerEvent const* pev)
{
    auto iev = mir_pointer_event_input_event(pev);
    auto pointer_action = mir_pointer_event_action(pev);

    if (pointer_action == mir_pointer_action_button_up ||
        pointer_action == mir_pointer_action_button_down)
    {
        last_input_event_timestamp = mir_input_event_get_event_time(iev);
    }
}

void me::BasicWindowManager::update_event_timestamp(MirTouchEvent const* tev)
{
    auto iev = mir_touch_event_input_event(tev);
    auto touch_count = mir_touch_event_point_count(tev);
    for (unsigned i = 0; i < touch_count; i++)
    {
        auto touch_action = mir_touch_event_action(tev, i);
        if (touch_action == mir_touch_action_up ||
            touch_action == mir_touch_action_down)
        {
            last_input_event_timestamp = mir_input_event_get_event_time(iev);
            break;
        }
    }
}
