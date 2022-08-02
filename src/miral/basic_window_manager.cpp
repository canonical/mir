/*
 * Copyright © 2015-2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "basic_window_manager.h"
#include "display_configuration_listeners.h"

#include "miral/window_manager_tools.h"

#include <mir/log.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/shell/display_layout.h>
#include <mir/shell/persistent_surface_store.h>
#include <mir/shell/surface_specification.h>

#include <boost/throw_exception.hpp>

#include <algorithm>

using namespace mir;
using namespace mir::geometry;

auto miral::BasicWindowManager::DisplayArea::bounding_rectangle_of_contained_outputs() const -> Rectangle
{
    Rectangles box;
    for (auto const& output : contained_outputs)
    {
        box.add(output.extents());
    }
    return box.bounding_rectangle();
}

auto miral::BasicWindowManager::DisplayArea::is_alive() const -> bool
{
    return !contained_outputs.empty();
}

struct miral::BasicWindowManager::Locker
{
    explicit Locker(miral::BasicWindowManager* self);

    ~Locker()
    {
        policy->advise_end();
    }

    std::lock_guard<std::mutex> const lock;
    WindowManagementPolicy* const policy;
};

miral::BasicWindowManager::Locker::Locker(BasicWindowManager* self) :
    lock{self->mutex},
    policy{self->policy.get()}
{
    policy->advise_begin();
    std::vector<std::weak_ptr<Workspace>> workspaces;
    {
        std::lock_guard const lock{self->dead_workspaces->dead_workspaces_mutex};
        workspaces.swap(self->dead_workspaces->workspaces);
    }

    for (auto const& workspace : workspaces)
        self->workspaces_to_windows.left.erase(workspace);
}

miral::BasicWindowManager::BasicWindowManager(
    shell::FocusController* focus_controller,
    std::shared_ptr<shell::DisplayLayout> const& display_layout,
    std::shared_ptr<mir::shell::PersistentSurfaceStore> const& persistent_surface_store,
    mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers,
    WindowManagementPolicyBuilder const& build) :
    focus_controller(focus_controller),
    display_layout(display_layout),
    persistent_surface_store{persistent_surface_store},
    policy(build(WindowManagerTools{this})),
    display_config_monitor{std::make_shared<DisplayConfigurationListeners>()}
{
    display_config_monitor->add_listener(this);
    display_configuration_observers.register_interest(display_config_monitor);
}

miral::BasicWindowManager::~BasicWindowManager()
{
    display_config_monitor->delete_listener(this);

    if (last_input_event)
        mir_event_unref(last_input_event);
}
void miral::BasicWindowManager::add_session(std::shared_ptr<scene::Session> const& session)
{
    Locker lock{this};
    policy->advise_new_app(app_info[session] = ApplicationInfo(session));
}

void miral::BasicWindowManager::remove_session(std::shared_ptr<scene::Session> const& session)
{
    Locker lock{this};
    auto info = app_info.find(session);
    if (info == app_info.end())
    {
        log_debug(
            "BasicWindowManager::remove_session() called with unknown or already removed session %s (PID: %d)",
            session->name().c_str(),
            session->process_id());
        return;
    }
    policy->advise_delete_app(info->second);
    app_info.erase(session);
}

auto miral::BasicWindowManager::add_surface(
    std::shared_ptr<scene::Session> const& session,
    shell::SurfaceSpecification const& params,
    std::function<std::shared_ptr<scene::Surface>(
        std::shared_ptr<scene::Session> const& session,
        shell::SurfaceSpecification const& params)> const& build)
-> std::shared_ptr<scene::Surface>
{
    Locker lock{this};

    auto& session_info = info_for(session);

    WindowSpecification spec = policy->place_new_window(session_info, place_new_surface(params));

    auto const surface = build(session, make_surface_spec(spec));
    Window const window{session, surface};
    auto& window_info = this->window_info.emplace(window, WindowInfo{window, spec}).first->second;

    session_info.add_window(window);

    auto const parent_surface = spec.parent().is_set() ? spec.parent().value().lock() : nullptr;
    auto const parent = parent_surface ? info_for(parent_surface).window() : Window{};
    window_info.parent(parent);
    if (parent)
    {
        info_for(parent).add_child(window);
    }

    if (spec.userdata().is_set())
        window_info.userdata() = spec.userdata().value();

    for_each_workspace_containing(parent,
        [&](std::shared_ptr<miral::Workspace> const& workspace) { add_tree_to_workspace(window, workspace); });

    update_attached_and_fullscreen_sets(window_info);

    if (window_info.state() == mir_window_state_attached)
        update_application_zones_and_attached_windows();

    policy->advise_new_window(window_info);

    if (parent && spec.aux_rect().is_set() && spec.placement_hints().is_set())
    {
        Rectangle relative_placement{window.top_left() - (parent.top_left()-Point{}), window.size()};
        auto const mir_surface = std::shared_ptr<scene::Surface>(window);
        mir_surface->placed_relative(relative_placement);
    }

    return surface;
}

void miral::BasicWindowManager::surface_ready(std::shared_ptr<scene::Surface> const& surface)
{
    Locker lock{this};
    policy->handle_window_ready(info_for(surface));
}

void miral::BasicWindowManager::modify_surface(
    std::shared_ptr<scene::Session> const& /*application*/,
    std::shared_ptr<scene::Surface> const& surface,
    shell::SurfaceSpecification const& modifications)
{
    Locker lock{this};
    if (!surface_known(surface, "modify"))
    {
        return;
    }
    auto& info = info_for(surface);
    WindowSpecification mods{modifications};
    validate_modification_request(mods, info);
    place_and_size_for_state(mods, info);
    policy->handle_modify_window(info, mods);
}

void miral::BasicWindowManager::remove_surface(
    std::shared_ptr<scene::Session> const& session,
    std::weak_ptr<scene::Surface> const& surface)
{
    Locker lock{this};
    if (app_info.find(session) == app_info.end())
    {
        log_debug(
            "BasicWindowManager::remove_surface() called with unknown or already removed session %s (PID: %d)",
            session->name().c_str(),
            session->process_id());
        return;
    }

    if (!surface_known(surface, "remove"))
        return;

    remove_window(session, info_for(surface));
}

void miral::BasicWindowManager::remove_window(Application const& application, miral::WindowInfo const& info)
{
    bool const is_active_window{active_window() == info.window()};
    auto const workspaces_containing_window = workspaces_containing(info.window());

    {
        std::vector<Window> const windows_removed{info.window()};

        for (auto const& workspace : workspaces_containing_window)
        {
            policy->advise_removing_from_workspace(workspace, windows_removed);
        }

        workspaces_to_windows.right.erase(info.window());
    }

    policy->advise_delete_window(info);

    info_for(application).remove_window(info.window());
    mru_active_windows.erase(info.window());
    fullscreen_surfaces.erase(info.window());
    for (auto& area : display_areas)
        area->attached_windows.erase(info.window());
    if (info.state() == mir_window_state_attached &&
        info.exclusive_rect().is_set())
    {
        update_application_zones_and_attached_windows();
    }

    application->destroy_surface(info.window());

    // NB erase() invalidates info, but we want to keep access to "parent".
    auto const parent = info.parent();
    erase(info);

    if (is_active_window)
    {
        refocus(application, parent, workspaces_containing_window);
    }
}

void miral::BasicWindowManager::refocus(
    miral::Application const& application, miral::Window const& hint,
    std::vector<std::shared_ptr<Workspace>> const& workspaces_containing_window)
{
    // If there's a hint, try using that first
    if (hint && select_active_window(hint))
        return;

    if (can_activate_window_for_session_in_workspace(application, workspaces_containing_window))
        return;

    // Try to activate to recently active window of any application in a shared workspace
    {
        miral::Window new_focus;

        mru_active_windows.enumerate([&](miral::Window& window)
            {
                // select_active_window() calls set_focus_to() which updates mru_active_windows and changes window
                auto const w = window;

                for (auto const& workspace : workspaces_containing(w))
                {
                    for (auto const& ww : workspaces_containing_window)
                    {
                        if (ww == workspace)
                        {
                            return !(new_focus = select_active_window(w));
                        }
                    }
                }

                return true;
            });

        if (new_focus) return;
    }

    if (can_activate_window_for_session(application))
        return;

    // Try to activate to recently active window of any application
    {
        miral::Window new_focus;

        mru_active_windows.enumerate([&](miral::Window& window)
            {
                // select_active_window() calls set_focus_to() which updates mru_active_windows and changes window
                auto const w = window;
                return !(new_focus = select_active_window(w));
            });

        if (new_focus) return;
    }

    // Fallback to cycling through applications
    focus_next_application();
}

void miral::BasicWindowManager::erase(miral::WindowInfo const& info)
{
    if (auto const parent = info.parent())
        info_for(parent).remove_child(info.window());

    for (auto& child : info.children())
        info_for(child).parent({});

    window_info.erase(info.window());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void miral::BasicWindowManager::add_display(geometry::Rectangle const& /*area*/)
{
    // OBSOLETE
}
#pragma GCC diagnostic pop

void miral::BasicWindowManager::remove_display(geometry::Rectangle const& /*area*/)
{
    // OBSOLETE
}

bool miral::BasicWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    Locker lock{this};
    update_event_timestamp(event);
    return policy->handle_keyboard_event(event);
}

bool miral::BasicWindowManager::handle_touch_event(MirTouchEvent const* event)
{
    Locker lock{this};
    update_event_timestamp(event);
    return policy->handle_touch_event(event);
}

bool miral::BasicWindowManager::handle_pointer_event(MirPointerEvent const* event)
{
    Locker lock{this};
    update_event_timestamp(event);

    cursor = {
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    return policy->handle_pointer_event(event);
}

void miral::BasicWindowManager::handle_raise_surface(
    std::shared_ptr<scene::Session> const& /*application*/,
    std::shared_ptr<scene::Surface> const& surface,
    uint64_t timestamp)
{
    Locker lock{this};

    if (!surface_known(surface, "raise"))
        return;

    if (timestamp >= last_input_event_timestamp)
        policy->handle_raise_window(info_for(surface));
}

void miral::BasicWindowManager::handle_request_drag_and_drop(
    std::shared_ptr<mir::scene::Session> const& /*session*/,
    std::shared_ptr<mir::scene::Surface> const& surface,
    uint64_t timestamp)
{
    Locker lock{this};

    if (!surface_known(surface, "drag-and-drop"))
        return;

    if (timestamp >= last_input_event_timestamp)
        policy->handle_request_drag_and_drop(info_for(surface));
}

void miral::BasicWindowManager::handle_request_move(
    std::shared_ptr<mir::scene::Session> const& /*session*/,
    std::shared_ptr<mir::scene::Surface> const& surface,
    uint64_t timestamp)
{
    std::lock_guard lock(mutex);

    if (!surface_known(surface, "move"))
        return;

    if (timestamp >= last_input_event_timestamp && last_input_event)
    {
        policy->handle_request_move(info_for(surface), mir_event_get_input_event(last_input_event));
    }
}

void miral::BasicWindowManager::handle_request_resize(
    std::shared_ptr<mir::scene::Session> const& /*session*/,
    std::shared_ptr<mir::scene::Surface> const& surface,
    uint64_t timestamp,
    MirResizeEdge edge)
{
    std::lock_guard lock(mutex);

    if (!surface_known(surface, "resize"))
        return;

    if (timestamp >= last_input_event_timestamp && last_input_event)
    {
        policy->handle_request_resize(info_for(surface), mir_event_get_input_event(last_input_event), edge);
    }
}

int miral::BasicWindowManager::set_surface_attribute(
    std::shared_ptr<scene::Session> const& /*application*/,
    std::shared_ptr<scene::Surface> const& surface,
    MirWindowAttrib attrib,
    int value)
{
    WindowSpecification modification;
    switch (attrib)
    {
    case mir_window_attrib_type:
        modification.type() = MirWindowType(value);
        break;
    case mir_window_attrib_state:
        modification.state() = MirWindowState(value);
        break;

    case mir_window_attrib_preferred_orientation:
        modification.preferred_orientation() = MirOrientationMode(value);
        break;

    case mir_window_attrib_visibility:
        // The client really shouldn't be trying to set this.
        // But, as the legacy API exists, we treat it as a query
        return surface->query(mir_window_attrib_visibility);

    case mir_window_attrib_focus:
        // The client really shouldn't be trying to set this.
        // But, as the legacy API exists, we treat it as a query
        return surface->query(mir_window_attrib_focus);

    case mir_window_attrib_swapinterval:
    case mir_window_attrib_dpi:
    default:
        return surface->configure(attrib, value);
    }

    Locker lock{this};
    if (!surface_known(surface, "set attribute"))
        return 0;

    auto& info = info_for(surface);

    validate_modification_request(modification, info);
    place_and_size_for_state(modification, info);
    policy->handle_modify_window(info, modification);

    switch (attrib)
    {
    case mir_window_attrib_type:
        return info.type();

    case mir_window_attrib_state:
        return info.state();

    case mir_window_attrib_preferred_orientation:
        return info.preferred_orientation();
        break;

    default:
        return 0; // Can't get here anyway
    }
}

auto miral::BasicWindowManager::count_applications() const
-> unsigned int
{
    return app_info.size();
}


void miral::BasicWindowManager::for_each_application(std::function<void(ApplicationInfo& info)> const& functor)
{
    for(auto& info : app_info)
    {
        functor(info.second);
    }
}

auto miral::BasicWindowManager::find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
-> Application
{
    for(auto& info : app_info)
    {
        if (predicate(info.second))
        {
            return Application{info.first};
        }
    }

    return Application{};
}

auto miral::BasicWindowManager::info_for(std::weak_ptr<scene::Session> const& session) const
-> ApplicationInfo&
{
    return const_cast<ApplicationInfo&>(app_info.at(session));
}

auto miral::BasicWindowManager::info_for(std::weak_ptr<scene::Surface> const& surface) const
-> WindowInfo&
{
    return const_cast<WindowInfo&>(window_info.at(surface));
}

auto miral::BasicWindowManager::info_for(Window const& window) const
-> WindowInfo&
{
    return info_for(std::weak_ptr<mir::scene::Surface>(window));
}

void miral::BasicWindowManager::ask_client_to_close(Window const& window)
{
    if (auto const mir_surface = std::shared_ptr<scene::Surface>(window))
        mir_surface->request_client_surface_close();
}

auto miral::BasicWindowManager::active_window() const -> Window
{
    return allow_active_window ? mru_active_windows.top() : Window{};
}

void miral::BasicWindowManager::focus_next_application()
{
    if (auto const prev = active_window())
    {
        auto const workspaces_containing_window = workspaces_containing(prev);

        if (!workspaces_containing_window.empty())
        {
            do
            {
                focus_controller->focus_next_session();

                if (can_activate_window_for_session_in_workspace(
                    focus_controller->focused_session(),
                    workspaces_containing_window))
                {
                    return;
                }
            }
            while (focus_controller->focused_session() != prev.application());
        }
        else
        {
            do
            {
                focus_controller->focus_next_session();

                if (can_activate_window_for_session(focus_controller->focused_session()))
                {
                    return;
                }
            }
            while (focus_controller->focused_session() != prev.application());
        }
    }
    else
    {
        focus_controller->focus_next_session();

        if (can_activate_window_for_session(focus_controller->focused_session()))
        {
            return;
        }
    }

    // Last resort: accept wherever focus_controller places focus
    auto const focussed_surface = focus_controller->focused_surface();
    select_active_window(focussed_surface ? info_for(focussed_surface).window() : Window{});
}

void miral::BasicWindowManager::focus_prev_application()
{
    if (auto const prev = active_window())
    {
        auto const workspaces_containing_window = workspaces_containing(prev);

        if (!workspaces_containing_window.empty())
        {
            do
            {
                focus_controller->focus_prev_session();

                if (can_activate_window_for_session_in_workspace(
                    focus_controller->focused_session(),
                    workspaces_containing_window))
                {
                    return;
                }
            }
            while (focus_controller->focused_session() != prev.application());
        }
        else
        {
            do
            {
                focus_controller->focus_prev_session();

                if (can_activate_window_for_session(focus_controller->focused_session()))
                {
                    return;
                }
            }
            while (focus_controller->focused_session() != prev.application());
        }
    }
    else
    {
        focus_controller->focus_prev_session();

        if (can_activate_window_for_session(focus_controller->focused_session()))
        {
            return;
        }
    }

    // Last resort: accept wherever focus_controller places focus
    auto const focussed_surface = focus_controller->focused_surface();
    select_active_window(focussed_surface ? info_for(focussed_surface).window() : Window{});
}

auto miral::BasicWindowManager::workspaces_containing(Window const& window) const
-> std::vector<std::shared_ptr<Workspace>>
{
    auto const iter_pair = workspaces_to_windows.right.equal_range(window);

    std::vector<std::shared_ptr<Workspace>> workspaces_containing_window;
    for (auto kv = iter_pair.first; kv != iter_pair.second; ++kv)
    {
        if (auto const workspace = kv->second.lock())
        {
            workspaces_containing_window.push_back(workspace);
        }
    }

    return workspaces_containing_window;
}

auto miral::BasicWindowManager::active_display_area() const -> std::shared_ptr<DisplayArea>
{
    // If a window has input focus, return its display area
    if (auto const surface = focus_controller->focused_surface())
    {
        WindowInfo& info = info_for(surface);
        return display_area_for(info);
    }

    // Otherwise, the display that contains the pointer, if there is one.
    for (auto const& area : display_areas)
    {
        if (area->area.contains(cursor))
        {
            // Ignore the (unspecified) possiblity of overlapping areas
            return area;
        }
    }

    // Otherwise, the first display.
    if (!display_areas.empty())
        return *display_areas.begin();

    return std::make_shared<DisplayArea>(Rectangle{{0, 0}, {100, 100}});
}

auto miral::BasicWindowManager::display_area_for_output_id(int output_id) const -> std::shared_ptr<DisplayArea>
{
    for (auto const& area : display_areas)
    {
        for (auto const& output : area->contained_outputs)
        {
            if (output.id() == output_id)
            {
                return area;
            }
        }
    }

    return {};
}

auto miral::BasicWindowManager::display_area_for(WindowInfo const& info) const -> std::shared_ptr<DisplayArea>
{
    auto const window = info.window();

    if (info.has_output_id())
    {
        if (auto const area = display_area_for_output_id(info.output_id()))
        {
            return area;
        }
    }

    for (auto& area : display_areas)
    {
        for (auto& area_window : area->attached_windows)
        {
            if (area_window == window)
                return area;
        }
    }

    // If the window is not explicity attached to any area, find the area it overlaps most with
    Rectangle window_rect{window.top_left(), window.size()};

    if (auto best_area = display_area_for(window_rect))
        return best_area.value();
    else
        return std::make_shared<DisplayArea>(window_rect);
}

auto miral::BasicWindowManager::display_area_for(Rectangle const& rect) const
-> std::optional<std::shared_ptr<DisplayArea>>
{
    int max_overlap_area = 0;
    int min_distance = INT_MAX;
    std::optional<std::shared_ptr<DisplayArea>> best_area;
    for (auto& area : display_areas)
    {
        auto const intersection = rect.intersection_with(area->area).size;
        auto const intersection_area = intersection.width.as_int() * intersection.height.as_int();
        if (intersection_area > max_overlap_area)
        {
            // Find the area with the biggest overlap
            best_area = area;
            max_overlap_area = intersection_area;
        }
        else if (max_overlap_area == 0)
        {
            // or if none overlap, find the area that is closest
            auto distance = std::max(
                std::max(
                    rect.left() - area->area.right(),
                    area->area.left() - rect.right()).as_int(),
                std::max(
                    rect.top() - area->area.bottom(),
                    area->area.top() - rect.bottom()).as_int());
            if (distance < min_distance)
            {
                best_area = area;
                min_distance = distance;
            }
        }
    }
    return best_area;
}

void miral::BasicWindowManager::focus_next_within_application()
{
    if (auto const prev = active_window())
    {
        auto const workspaces_containing_window = workspaces_containing(prev);
        auto const& siblings = info_for(prev.application()).windows();
        auto current = find(begin(siblings), end(siblings), prev);

        if (current != end(siblings))
        {
            while (++current != end(siblings))
            {
                for (auto const& workspace : workspaces_containing(*current))
                {
                    for (auto const& ww : workspaces_containing_window)
                    {
                        if (ww == workspace)
                        {
                            if (prev != select_active_window(*current))
                                return;
                        }
                    }
                }
            }
        }

        for (current = begin(siblings); *current != prev; ++current)
        {
            for (auto const& workspace : workspaces_containing(*current))
            {
                for (auto const& ww : workspaces_containing_window)
                {
                    if (ww == workspace)
                    {
                        if (prev != select_active_window(*current))
                            return;
                    }
                }
            }
        }

        current = find(begin(siblings), end(siblings), prev);
        if (current != end(siblings))
        {
            while (++current != end(siblings) && prev == select_active_window(*current))
                ;
        }

        if (current == end(siblings))
        {
            current = begin(siblings);
            while (prev != *current && prev == select_active_window(*current))
                ++current;
        }
    }
}

void miral::BasicWindowManager::focus_prev_within_application()
{
    if (auto const prev = active_window())
    {
        auto const workspaces_containing_window = workspaces_containing(prev);
        auto const& siblings = info_for(prev.application()).windows();
        auto current = find(rbegin(siblings), rend(siblings), prev);

        if (current != rend(siblings))
        {
            while (++current != rend(siblings))
            {
                for (auto const& workspace : workspaces_containing(*current))
                {
                    for (auto const& ww : workspaces_containing_window)
                    {
                        if (ww == workspace)
                        {
                            if (prev != select_active_window(*current))
                                return;
                        }
                    }
                }
            }
        }

        for (current = rbegin(siblings); *current != prev; ++current)
        {
            for (auto const& workspace : workspaces_containing(*current))
            {
                for (auto const& ww : workspaces_containing_window)
                {
                    if (ww == workspace)
                    {
                        if (prev != select_active_window(*current))
                            return;
                    }
                }
            }
        }

        current = find(rbegin(siblings), rend(siblings), prev);
        if (current != rend(siblings))
        {
            while (++current != rend(siblings) && prev == select_active_window(*current))
                ;
        }

        if (current == rend(siblings))
        {
            current = rbegin(siblings);
            while (prev != *current && prev == select_active_window(*current))
                ++current;
        }
    }
}

auto miral::BasicWindowManager::window_at(geometry::Point cursor) const
-> Window
{
    auto surface_at = focus_controller->surface_at(cursor);
    return surface_at ? info_for(surface_at).window() : Window{};
}

auto miral::BasicWindowManager::active_output() -> geometry::Rectangle const
{
    return active_display_area()->area;
}

auto miral::BasicWindowManager::active_application_zone() -> Zone
{
    return active_display_area()->application_zone;
}

void miral::BasicWindowManager::raise_tree(Window const& root)
{
    auto const& info = info_for(root);

    if (auto parent = info.parent())
        raise_tree(parent);

    std::vector<Window> windows;

    std::function<void(WindowInfo const& info)> const add_children =
        [&,this](WindowInfo const& info)
            {
                for (auto const& child : info.children())
                {
                    windows.push_back(child);
                    add_children(info_for(child));
                }
            };

    windows.push_back(root);
    add_children(info);

    policy->advise_raise(windows);
    focus_controller->raise({begin(windows), end(windows)});
}

void miral::BasicWindowManager::start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle)
{
    std::shared_ptr<scene::Surface>(window_info.window())->start_drag_and_drop(handle);
    focus_controller->set_drag_and_drop_handle(handle);
}

void miral::BasicWindowManager::end_drag_and_drop()
{
    focus_controller->clear_drag_and_drop_handle();
}

void miral::BasicWindowManager::move_tree(miral::WindowInfo& root, mir::geometry::Displacement movement)
{
    if (movement == mir::geometry::Displacement{})
        return;

    auto const top_left = root.window().top_left() + movement;

    policy->advise_move_to(root, top_left);
    root.window().move_to(top_left);

    for (auto const& child: root.children())
    {
        auto const& pos = policy->confirm_inherited_move(info_for(child), movement);
        place_and_size(info_for(child), pos.top_left, pos.size);
    }
}

void miral::BasicWindowManager::set_tree_depth_layer(miral::WindowInfo& root, MirDepthLayer new_layer)
{
    std::shared_ptr<scene::Surface> surface = root.window();
    if (surface->depth_layer() != new_layer || root.depth_layer() != new_layer)
    {
        surface->set_depth_layer(new_layer);
        root.depth_layer(new_layer);
        for (auto& window : root.children())
        {
            set_tree_depth_layer(info_for(window), new_layer);
        }
    }
}

void miral::BasicWindowManager::modify_window(WindowInfo& window_info, WindowSpecification const& modifications)
{
    WindowInfo window_info_tmp{window_info};

#define COPY_IF_SET(field)\
    if (modifications.field().is_set())\
        window_info_tmp.field(modifications.field().value())

    COPY_IF_SET(name);
    COPY_IF_SET(type);
    // state is handled "differently" and only updated by set_state() at the end
    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(min_aspect);
    COPY_IF_SET(max_aspect);
    COPY_IF_SET(output_id);
    COPY_IF_SET(preferred_orientation);
    COPY_IF_SET(confine_pointer);
    COPY_IF_SET(userdata);
    COPY_IF_SET(shell_chrome);
    COPY_IF_SET(depth_layer);
    COPY_IF_SET(attached_edges);
    COPY_IF_SET(exclusive_rect);
    COPY_IF_SET(application_id);
    COPY_IF_SET(focus_mode);

#undef COPY_IF_SET

    if (modifications.parent().is_set())
    {
        if (auto const& parent_window = modifications.parent().value().lock())
        {
            window_info_tmp.parent(info_for(parent_window).window());
        }
        else
        {
            window_info_tmp.parent(Window{});
        }
    }

    if (window_info.type() != window_info_tmp.type())
    {
        auto const new_type = window_info_tmp.type();

        if (!window_info.can_morph_to(new_type))
        {
            throw std::runtime_error("Unsupported window type change");
        }

        if (window_info_tmp.must_not_have_parent())
        {
            if (modifications.parent().is_set())
                throw std::runtime_error("Target window type does not support parent");

            window_info_tmp.parent({});
        }
        else if (window_info_tmp.must_have_parent())
        {
            if (!window_info_tmp.parent())
                throw std::runtime_error("Target window type requires parent");
        }
    }

    bool application_zones_need_update = false;
    if (window_info.state() == mir_window_state_attached ||
          (modifications.state().is_set() &&
           modifications.state().value() == mir_window_state_attached))
    {
        if (modifications.state().is_set() ||
            modifications.size().is_set() ||
            modifications.attached_edges().is_set() ||
            modifications.exclusive_rect().is_set())
        {
            application_zones_need_update = true;
        }
    }

    std::swap(window_info_tmp, window_info);

    auto& window = window_info.window();

    if (modifications.depth_layer().is_set())
        set_tree_depth_layer(window_info, modifications.depth_layer().value());

    if (window_info.type() != window_info_tmp.type())
        std::shared_ptr<scene::Surface>(window)->configure(mir_window_attrib_type, window_info.type());

    if (window_info.parent() != window_info_tmp.parent())
    {
        if (window_info_tmp.parent())
        {
            auto& parent_info = info_for(window_info_tmp.parent());
            parent_info.remove_child(window);
        }

        if (window_info.parent())
        {
            auto& parent_info = info_for(window_info.parent());
            parent_info.add_child(window);
        }
    }

    if (modifications.name().is_set())
        std::shared_ptr<scene::Surface>(window)->rename(modifications.name().value());

    if (modifications.input_shape().is_set())
        std::shared_ptr<scene::Surface>(window)->set_input_region(modifications.input_shape().value());

    if (modifications.state().is_set() && window_info.state() != modifications.state().value())
    {
        switch (window_info.state())
        {
        case mir_window_state_restored:
        case mir_window_state_hidden:
            window_info.restore_rect({window.top_left(), window.size()});
            break;

        case mir_window_state_vertmaximized:
        {
            auto restore_rect = window_info.restore_rect();
            restore_rect.top_left.x = window.top_left().x;
            restore_rect.size.width = window.size().width;
            window_info.restore_rect(restore_rect);
            break;
        }

        case mir_window_state_horizmaximized:
        {
            auto restore_rect = window_info.restore_rect();
            restore_rect.top_left.y = window.top_left().y;
            restore_rect.size.height= window.size().height;
            window_info.restore_rect(restore_rect);
            break;
        }

        default:
            break;
        }
    }

    if (modifications.size().is_set() || modifications.top_left().is_set() ||
        modifications.min_width().is_set() || modifications.min_height().is_set() ||
        modifications.max_width().is_set() || modifications.max_height().is_set() ||
        modifications.width_inc().is_set() || modifications.height_inc().is_set())
    {
        Point new_pos = modifications.top_left().is_set() ? modifications.top_left().value() : window.top_left();
        Size new_size = modifications.size().is_set() ? modifications.size().value() : window.size();

        // The new size constraints have already been applied to window_info and constrain_resize() will use them
        window_info.constrain_resize(new_pos, new_size);
        place_and_size(window_info, new_pos, new_size);
    }

    if (modifications.placement_hints().is_set())
    {
        if (auto parent = window_info.parent())
        {
            std::shared_ptr<scene::Surface> const parent_scene_surface{parent};
            Rectangle const parent_content_area{
                parent_scene_surface->top_left() + parent_scene_surface->content_offset(),
                parent_scene_surface->content_size()};
            auto new_pos = place_relative(parent_content_area, modifications, window.size());

            if (new_pos.is_set())
                place_and_size(window_info, new_pos.value().top_left, new_pos.value().size);

            std::shared_ptr<scene::Surface> const child_scene_surface{window};
            Rectangle const relative_placement{
                child_scene_surface->top_left() - as_displacement(parent_content_area.top_left),
                child_scene_surface->content_size()};
            child_scene_surface->placed_relative(relative_placement);
        }
    }

    if (modifications.focus_mode().is_set())
    {
        switch (modifications.focus_mode().value())
        {
        case mir_focus_mode_grabbing:
            select_active_window(window);
            break;

        case mir_focus_mode_disabled:
            if (window == active_window())
            {
                refocus(window.application(), window_info.parent(), workspaces_containing(window));
            }

        default:;
        }
    }

    if (modifications.state().is_set())
    {
        set_state(window_info, modifications.state().value());
    }
    else if (modifications.output_id().is_set())
    {
        update_attached_and_fullscreen_sets(window_info);
        application_zones_need_update = true;
    }

    if (application_zones_need_update)
    {
        update_application_zones_and_attached_windows();
    }

    if (modifications.confine_pointer().is_set())
        std::shared_ptr<scene::Surface>(window)->set_confine_pointer_state(modifications.confine_pointer().value());
}

auto miral::BasicWindowManager::info_for_window_id(std::string const& id) const -> WindowInfo&
{
    auto surface = persistent_surface_store->surface_for_id(mir::shell::PersistentSurfaceStore::Id{id});

    if (!surface)
        BOOST_THROW_EXCEPTION(std::runtime_error{"No surface matching ID"});

    return info_for(surface);
}

auto miral::BasicWindowManager::id_for_window(Window const& window) const -> std::string
{
    if (!window)
        BOOST_THROW_EXCEPTION(std::runtime_error{"Null Window has no Persistent ID"});

    return persistent_surface_store->id_for_surface(window).serialize_to_string();
}

void miral::BasicWindowManager::place_and_size(WindowInfo& root, Point const& new_pos, Size const& new_size)
{
    if (root.window().size() != new_size)
    {
        policy->advise_resize(root, new_size);
        root.window().resize(new_size);
    }

    move_tree(root, new_pos - root.window().top_left());
}

void miral::BasicWindowManager::place_attached_to_zone(
    WindowInfo& info,
    Rectangle const& application_zone)
{
    Point top_left = info.window().top_left();
    Size size = info.window().size();
    bool update_window = false;

    switch (info.state())
    {
    case mir_window_state_maximized:
        top_left = application_zone.top_left;
        size = application_zone.size;
        update_window = true;
        break;

    case mir_window_state_horizmaximized:
        top_left.x = application_zone.top_left.x;
        size.width = application_zone.size.width;
        update_window = true;
        break;

    case mir_window_state_vertmaximized:
        top_left.y = application_zone.top_left.y;
        size.height = application_zone.size.height;
        update_window = true;
        break;

    case mir_window_state_attached:
    {
        MirPlacementGravity edges = info.attached_edges();
        Rectangle placement_zone = application_zone;

        if ((edges & mir_placement_gravity_west) &&
            (edges & mir_placement_gravity_east))
        {
            placement_zone.top_left.x = application_zone.top_left.x;
            placement_zone.size.width = application_zone.size.width;
            size.width = placement_zone.size.width;
        }

        if ((edges & mir_placement_gravity_north) &&
            (edges & mir_placement_gravity_south))
        {
            placement_zone.top_left.y = application_zone.top_left.y;
            placement_zone.size.height = application_zone.size.height;
            size.height = placement_zone.size.height;
        }

        if (edges & mir_placement_gravity_west)
            top_left.x = placement_zone.left();
        else if (edges & mir_placement_gravity_east)
            top_left.x = placement_zone.right() - (size.width - Width{0});
        else
            top_left.x = placement_zone.top_left.x + (placement_zone.size.width - size.width) * 0.5;

        if (edges & mir_placement_gravity_north)
            top_left.y = placement_zone.top();
        else if (edges & mir_placement_gravity_south)
            top_left.y = placement_zone.bottom() - (size.height - Height{0});
        else
            top_left.y = placement_zone.top_left.y + (placement_zone.size.height - size.height) * 0.5;
        update_window = true;
        break;
    }

    default:
        fatal_error("BasicWindowManager::place_attached_to_zone() called for window not in a maximized or attached state");
    }

    // TODO: Maybe remove update_window and update only if the rect has changed?
    // This would probably be more correct, but also may or may not break existing users

    if (update_window)
    {
        auto updated_rect = policy->confirm_placement_on_display(info, info.state(), Rectangle{top_left, size});
        place_and_size(info, updated_rect.top_left, updated_rect.size);
    }
}

void miral::BasicWindowManager::place_and_size_for_state(
    WindowSpecification& modifications, WindowInfo const& window_info) const
{
    if (!modifications.state().is_set())
        return;

    auto const new_state = modifications.state().value();

    switch (new_state)
    {
    case mir_window_state_fullscreen:
        if (modifications.output_id().is_set() &&
           (!window_info.has_output_id() || modifications.output_id().value() != window_info.output_id()))
                break;
        // Falls through.
    default:
        if (new_state == window_info.state())
            return;
    }

    auto const window = window_info.window();

    auto restore_rect = window_info.restore_rect();

    // window_info.restore_rect() was cached on last state change, update to reflect current window position
    switch (window_info.state())
    {
    case mir_window_state_restored:
    case mir_window_state_hidden:
        restore_rect = {window.top_left(), window.size()};
        break;

    case mir_window_state_vertmaximized:
    {
        restore_rect.top_left.x = window.top_left().x;
        restore_rect.size.width = window.size().width;
        break;
    }

    case mir_window_state_horizmaximized:
    {
        restore_rect.top_left.y = window.top_left().y;
        restore_rect.size.height= window.size().height;
        break;
    }

    default:
        break;
    }

    // If the shell has also set position, that overrides restore_rect default
    if (modifications.top_left().is_set())
        restore_rect.top_left = modifications.top_left().value();

    // If the client or shell has also set size, that overrides restore_rect default
    if (modifications.size().is_set())
        restore_rect.size = modifications.size().value();

    auto const display_area = display_area_for(window_info);
    auto const application_zone = display_area->application_zone.extents();
    Rectangle rect;

    switch (new_state)
    {
    case mir_window_state_restored:
        rect = restore_rect;
        break;

    case mir_window_state_maximized:
        rect = policy->confirm_placement_on_display(window_info, new_state, application_zone);
        break;

    case mir_window_state_horizmaximized:
        rect.top_left = {application_zone.left(), restore_rect.top()};
        rect.size = {application_zone.size.width, restore_rect.size.height};
        rect = policy->confirm_placement_on_display(window_info, new_state, rect);
        break;

    case mir_window_state_vertmaximized:
        rect.top_left = {restore_rect.left(), application_zone.top()};
        rect.size = {restore_rect.size.width, application_zone.size.height};
        rect = policy->confirm_placement_on_display(window_info, new_state, rect);
        break;

    case mir_window_state_fullscreen:
    {
        rect = policy->confirm_placement_on_display(window_info, new_state, display_area_for(window_info)->area);
        break;
    }

    case mir_window_state_hidden:
    case mir_window_state_minimized:
    default:
        return;
    }

    modifications.top_left() = rect.top_left;
    modifications.size() = rect.size;
}

void miral::BasicWindowManager::update_attached_and_fullscreen_sets(WindowInfo const& window_info)
{
    auto const state = window_info.state();
    auto const window = window_info.window();

    if (state == mir_window_state_fullscreen)
    {
        fullscreen_surfaces.insert(window);
    }
    else
    {
        fullscreen_surfaces.erase(window);
    }

    for (auto& area : display_areas)
        area->attached_windows.erase(window);

    switch (state)
    {
    case mir_window_state_maximized:
    case mir_window_state_vertmaximized:
    case mir_window_state_horizmaximized:
    case mir_window_state_attached:
    {
        auto area = display_area_for(window_info);
        area->attached_windows.insert(window);
        break;
    }

    default:;
    }
}

void miral::BasicWindowManager::set_state(miral::WindowInfo& window_info, MirWindowState value)
{
    if (window_info.state() == value)
    {
        return;
    }

    auto const window = window_info.window();
    auto const mir_surface = std::shared_ptr<scene::Surface>(window);
    bool const was_hidden = window_info.state() == mir_window_state_hidden ||
                            window_info.state() == mir_window_state_minimized;

    policy->advise_state_change(window_info, value);

    switch (value)
    {
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        // This is a bit hacky as it relies on non-local knowledge, but it works without adding complexity elsewhere.
        //
        // We set the state to mir_window_state_hidden (even if value is mir_window_state_minimized) before
        // looking for another window in the same "tree" to give focus. This means we can call the widely used
        // function select_active_window() which would otherwise restore the window we want to minimize.
        //
        // We set the correct state after working out the new active window.  alan_g
        window_info.state(mir_window_state_hidden);

        if (window == active_window())
        {
            select_active_window(window);

            if (window == active_window() || !active_window())
            {
                auto const workspaces_containing_window = workspaces_containing(window);

                // Try to activate to recently active window of any application
                mru_active_windows.enumerate([&](Window& candidate)
                    {
                        if (candidate == window)
                            return true;
                        auto const w = candidate;
                        for (auto const& workspace : workspaces_containing(w))
                        {
                            for (auto const& ww : workspaces_containing_window)
                            {
                                if (ww == workspace)
                                {
                                    return !(select_active_window(w));
                                }
                            }
                        }

                        return true;
                    });
            }

            // Try to activate to recently active window of any application
            if (window == active_window() || !active_window())
                mru_active_windows.enumerate([&](Window& candidate)
                {
                    if (candidate == window)
                        return true;
                    auto const w = candidate;
                    return !(select_active_window(w));
                });

            if (window == active_window())
                select_active_window({});
        }

        window_info.state(value);
        mir_surface->configure(mir_window_attrib_state, value);
        mir_surface->hide();

        break;

    default:
        bool const can_become_active =
            !active_window() ||
            active_window().application() == window.application();
        window_info.state(value);
        mir_surface->configure(mir_window_attrib_state, value);
        mir_surface->show();
        if (was_hidden && can_become_active)
        {
            select_active_window(window);
        }
    }

    update_attached_and_fullscreen_sets(window_info);
}

void miral::BasicWindowManager::update_event_timestamp(MirKeyboardEvent const* kev)
{
    update_event_timestamp(mir_keyboard_event_input_event(kev));
}

void miral::BasicWindowManager::update_event_timestamp(MirPointerEvent const* pev)
{
    auto pointer_action = mir_pointer_event_action(pev);

    if (pointer_action == mir_pointer_action_button_up ||
        pointer_action == mir_pointer_action_button_down)
    {
        update_event_timestamp(mir_pointer_event_input_event(pev));
    }
}

void miral::BasicWindowManager::update_event_timestamp(MirTouchEvent const* tev)
{
    auto touch_count = mir_touch_event_point_count(tev);
    for (unsigned i = 0; i < touch_count; i++)
    {
        auto touch_action = mir_touch_event_action(tev, i);
        if (touch_action == mir_touch_action_up ||
            touch_action == mir_touch_action_down)
        {
            update_event_timestamp(mir_touch_event_input_event(tev));
            break;
        }
    }
}

void miral::BasicWindowManager::update_event_timestamp(MirInputEvent const* iev)
{
    last_input_event_timestamp = mir_input_event_get_event_time(iev);

    if (last_input_event)
        mir_event_unref(last_input_event);
    last_input_event = mir_event_ref(mir_input_event_get_event(iev));
}

void miral::BasicWindowManager::invoke_under_lock(std::function<void()> const& callback)
{
    Locker lock{this};
    callback();
}

auto miral::BasicWindowManager::select_active_window(Window const& hint) -> miral::Window
{
    auto const prev_window = active_window();

    // Lomiri "selects" a null Window to implicitly disable the active window
    allow_active_window = hint;

    if (!hint)
    {
        if (prev_window)
        {
            focus_controller->set_focus_to(hint.application(), hint);
            policy->advise_focus_lost(info_for(prev_window));
        }

        return hint;
    }

    auto const prev_windows_focus_mode = prev_window ? info_for(prev_window).focus_mode() : mir_focus_mode_focusable;
    if (prev_windows_focus_mode == mir_focus_mode_grabbing)
    {
        return prev_window;
    }

    auto& info_for_hint = info_for(hint);

    if (info_for_hint.focus_mode() == mir_focus_mode_disabled)
    {
        if (prev_windows_focus_mode == mir_focus_mode_disabled)
        {
            return {};
        }
        else
        {
            return prev_window;
        }
    }

    for (auto const& child : info_for_hint.children())
    {
        auto const& info_for_child = info_for(child);

        if (info_for_child.type() == mir_window_type_dialog && info_for_child.is_visible())
            return select_active_window(child);
    }

    if (info_for_hint.can_be_active() && info_for_hint.state() != mir_window_state_hidden)
    {
        if (!info_for_hint.is_visible())
        {
            std::shared_ptr<scene::Surface> const& mir_surface = hint;
            if (info_for_hint.state() == mir_window_state_minimized)
            {
                std::shared_ptr<scene::Surface> const scene_surface{hint};
                auto const new_state = scene_surface->state_tracker().without(mir_window_state_minimized).active_state();
                policy->advise_state_change(info_for_hint, new_state);
                info_for_hint.state(new_state);
                mir_surface->configure(mir_window_attrib_state, new_state);
            }
            mir_surface->show();
        }

        mru_active_windows.push(hint);
        focus_controller->set_focus_to(hint.application(), hint);

        if (prev_window && prev_window != hint)
            policy->advise_focus_lost(info_for(prev_window));

        policy->advise_focus_gained(info_for_hint);
        return hint;
    }
    else
    {
        // Cannot have input focus - try the parent
        if (auto const parent = info_for_hint.parent())
            return select_active_window(parent);
    }

    return {};
}

void miral::BasicWindowManager::drag_active_window(mir::geometry::Displacement movement)
{
    auto const window = active_window();

    drag_window(window, movement);
}

void miral::BasicWindowManager::drag_window(miral::Window const& window, Displacement& movement)
{
    if (!window)
        return;

    auto& window_info = info_for(window);

    // When a surface is moved interactively
    // -------------------------------------
    // Regular, floating regular, dialog, and satellite surfaces should be user-movable.
    // Popups, glosses, and tips should not be.
    // Freestyle surfaces may or may not be, as specified by the app.
    //                              Mir and Unity: Surfaces, input, and displays (v0.3)
    switch (window_info.type())
    {
    case mir_window_type_normal:   // regular
    case mir_window_type_utility:  // floating regular
    case mir_window_type_dialog:   // dialog
    case mir_window_type_satellite:// satellite
    case mir_window_type_freestyle:// freestyle
        break;

    case mir_window_type_gloss:
    case mir_window_type_menu:
    case mir_window_type_inputmethod:
    case mir_window_type_tip:
    case mir_window_type_decoration:
    case mir_window_types:
        return;
    }

    switch (window_info.state())
    {
    case mir_window_state_restored:
        break;

        // "A vertically maximised window is anchored to the top and bottom of
        // the available workspace and can have any width."
    case mir_window_state_vertmaximized:
        movement.dy = DeltaY(0);
        break;

        // "A horizontally maximised window is anchored to the left and right of
        // the available workspace and can have any height"
    case mir_window_state_horizmaximized:
        movement.dx = DeltaX(0);
        break;

        // "A maximised window is anchored to the top, bottom, left and right of the
        // available workspace. For example, if the launcher is always-visible then
        // the left-edge of the window is anchored to the right-edge of the launcher."
    case mir_window_state_maximized:
    case mir_window_state_fullscreen:
    default:
        return;
    }

    move_tree(window_info, movement);
}

auto miral::BasicWindowManager::surface_known(
    std::weak_ptr<scene::Surface> const& surface,
    std::string const& action) -> bool
{
    if (window_info.find(surface) != window_info.end())
    {
        return true;
    }
    else
    {
        std::string description;

        if (auto const shared = surface.lock())
            description = "unknown or removed surface " + shared->name();
        else
            description = "null surface";

        log_debug(
            "%s requested on %s",
            action.c_str(),
            description.c_str());

        return false;
    }
}

auto miral::BasicWindowManager::can_activate_window_for_session(miral::Application const& session) -> bool
{
    miral::Window new_focus;

    mru_active_windows.enumerate([&](miral::Window& window)
        {
            // select_active_window() calls set_focus_to() which updates mru_active_windows and changes window
            auto const w = window;
            return w.application() != session || !(new_focus = select_active_window(w));
        });

    return new_focus;
}

auto miral::BasicWindowManager::can_activate_window_for_session_in_workspace(
    Application const& session,
    std::vector<std::shared_ptr<Workspace>> const& workspaces) -> bool
{
    miral::Window new_focus;

    mru_active_windows.enumerate([&](miral::Window& window)
        {
            // select_active_window() calls set_focus_to() which updates mru_active_windows and changes window
            auto const w = window;

            if (w.application() != session)
                return true;

            for (auto const& workspace : workspaces_containing(w))
            {
                for (auto const& ww : workspaces)
                {
                    if (ww == workspace)
                        return !(new_focus = select_active_window(w));
                }
            }

            return true;
        });

    return new_focus;
}

auto miral::BasicWindowManager::place_new_surface(WindowSpecification parameters) -> WindowSpecification
{
    if (!parameters.type().is_set())
        parameters.type() = mir_window_type_normal;

    if (!parameters.state().is_set())
        parameters.state() = mir_window_state_restored;

    if (!parameters.size().is_set())
    {
        parameters.size() = {640, 480};
    }

    std::shared_ptr<DisplayArea> display_area;
    if (parameters.output_id().is_set())
    {
        display_area = display_area_for_output_id(parameters.output_id().value());
    }
    if (!display_area)
    {
        display_area = active_display_area();
    }

    auto const application_zone = display_area->application_zone.extents();

    bool positioned = false;

    std::shared_ptr<scene::Surface> parent_scene_surface =
        parameters.parent().is_set() ?
        parameters.parent().value().lock() :
        nullptr;

    if (parent_scene_surface)
    {
        if (parameters.aux_rect().is_set() && parameters.placement_hints().is_set())
        {
            Rectangle const parent_content_area{
                parent_scene_surface->top_left() + parent_scene_surface->content_offset(),
                parent_scene_surface->content_size()};

            auto const position = place_relative(
                parent_content_area,
                parameters,
                parameters.size().value());

            if (position.is_set())
            {
                parameters.top_left() = position.value().top_left;
                parameters.size() = position.value().size;
                positioned = true;
            }
        }
        else
        {
            //  o Otherwise, if the dialog is not the same as any previous dialog for the
            //    same parent window, and/or it does not have user-customized position:
            //      o It should be optically centered relative to its parent, unless this
            //        would overlap or cover the title bar of the parent.
            //      o Otherwise, it should be cascaded vertically (but not horizontally)
            //        relative to its parent, unless, this would cause at least part of
            //        it to extend into shell space.
            auto const parent = info_for(parameters.parent().value()).window();
            auto const parent_top_left = parent.top_left();
            auto const centred = parent_top_left
                                 + 0.5*(as_displacement(parent.size()) - as_displacement(parameters.size().value()))
                                 - (as_delta(parent.size().height) - as_delta(parameters.size().value().height)) / 6;

            parameters.top_left() = centred;
            positioned = true;
        }

        if (!parameters.depth_layer().is_set())
        {
            parameters.depth_layer() = parent_scene_surface->depth_layer();
        }

        if (!parameters.focus_mode().is_set())
        {
            parameters.focus_mode() = parent_scene_surface->focus_mode();
        }
    }

    if (!positioned)
    {
        auto centred = application_zone.top_left
                       + 0.5*(as_displacement(application_zone.size) - as_displacement(parameters.size().value()))
                       - (as_delta(application_zone.size.height) - as_delta(parameters.size().value().height)) / 6;

        switch (parameters.state().value())
        {
        case mir_window_state_fullscreen:
            parameters.top_left() = display_area->area.top_left;
            parameters.size() = display_area->area.size;
            break;

        case mir_window_state_maximized:
            parameters.top_left() = application_zone.top_left;
            parameters.size() = application_zone.size;
            break;

        case mir_window_state_vertmaximized:
            centred.y = application_zone.top_left.y;
            parameters.top_left() = centred;
            parameters.size() = Size{parameters.size().value().width, application_zone.size.height};
            break;

        case mir_window_state_horizmaximized:
            centred.x = application_zone.top_left.x;
            parameters.top_left() = centred;
            parameters.size() = Size{application_zone.size.width, parameters.size().value().height};
            break;

        default:
            parameters.top_left() = centred;
        }

        auto const display_area = outputs.bounding_rectangle();

        if (parameters.top_left().value().y < display_area.top_left.y)
            parameters.top_left() = Point{parameters.top_left().value().x, display_area.top_left.y};
    }

    return parameters;
}

namespace
{
auto flip_x(MirPlacementGravity rect_gravity) -> MirPlacementGravity
{
    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return mir_placement_gravity_northeast;

    case mir_placement_gravity_northeast:
        return mir_placement_gravity_northwest;

    case mir_placement_gravity_west:
        return mir_placement_gravity_east;

    case mir_placement_gravity_east:
        return mir_placement_gravity_west;

    case mir_placement_gravity_southwest:
        return mir_placement_gravity_southeast;

    case mir_placement_gravity_southeast:
        return mir_placement_gravity_southwest;

    default:
        return rect_gravity;
    }
}

auto flip_y(MirPlacementGravity rect_gravity) -> MirPlacementGravity
{
    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return mir_placement_gravity_southwest;

    case mir_placement_gravity_north:
        return mir_placement_gravity_south;

    case mir_placement_gravity_northeast:
        return mir_placement_gravity_southeast;

    case mir_placement_gravity_southwest:
        return mir_placement_gravity_northwest;

    case mir_placement_gravity_south:
        return mir_placement_gravity_north;

    case mir_placement_gravity_southeast:
        return mir_placement_gravity_northeast;

    default:
        return rect_gravity;
    }
}

auto flip_x(Displacement const& d) -> Displacement { return {-1*d.dx, d.dy}; }
auto flip_y(Displacement const& d) -> Displacement { return {d.dx, -1*d.dy}; }

auto antipodes(MirPlacementGravity rect_gravity) -> MirPlacementGravity
{
    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return mir_placement_gravity_southeast;

    case mir_placement_gravity_north:
        return mir_placement_gravity_south;

    case mir_placement_gravity_northeast:
        return mir_placement_gravity_southwest;

    case mir_placement_gravity_west:
        return mir_placement_gravity_east;

    case mir_placement_gravity_east:
        return mir_placement_gravity_west;

    case mir_placement_gravity_southwest:
        return mir_placement_gravity_northeast;

    case mir_placement_gravity_south:
        return mir_placement_gravity_north;

    case mir_placement_gravity_southeast:
        return mir_placement_gravity_northwest;

    default:
        return rect_gravity;
    }
}

auto constrain_to(mir::geometry::Rectangle const& rect, Point point) -> Point
{
    if (point.x < rect.top_left.x)
        point.x = rect.top_left.x;

    if (point.y < rect.top_left.y)
        point.y = rect.top_left.y;

    if (point.x > rect.bottom_right().x)
        point.x = rect.bottom_right().x;

    if (point.y > rect.bottom_right().y)
        point.y = rect.bottom_right().y;

    return point;
}

auto anchor_for(Rectangle const& aux_rect, MirPlacementGravity rect_gravity) -> Point
{
    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return aux_rect.top_left;

    case mir_placement_gravity_north:
        return aux_rect.top_left + 0.5*as_displacement(aux_rect.size).dx;

    case mir_placement_gravity_northeast:
        return aux_rect.top_right();

    case mir_placement_gravity_west:
        return aux_rect.top_left + 0.5*as_displacement(aux_rect.size).dy;

    case mir_placement_gravity_center:
        return aux_rect.top_left + 0.5*as_displacement(aux_rect.size);

    case mir_placement_gravity_east:
        return aux_rect.top_right() + 0.5*as_displacement(aux_rect.size).dy;

    case mir_placement_gravity_southwest:
        return aux_rect.bottom_left();

    case mir_placement_gravity_south:
        return aux_rect.bottom_left() + 0.5*as_displacement(aux_rect.size).dx;

    case mir_placement_gravity_southeast:
        return aux_rect.bottom_right();

    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("bad placement gravity"));
    }
}

auto offset_for(Size const& size, MirPlacementGravity rect_gravity) -> Displacement
{
    auto const displacement = as_displacement(size);

    switch (rect_gravity)
    {
    case mir_placement_gravity_northwest:
        return {0, 0};

    case mir_placement_gravity_north:
        return {-0.5 * displacement.dx, 0};

    case mir_placement_gravity_northeast:
        return {-1 * displacement.dx, 0};

    case mir_placement_gravity_west:
        return {0, -0.5 * displacement.dy};

    case mir_placement_gravity_center:
        return {-0.5 * displacement.dx, -0.5 * displacement.dy};

    case mir_placement_gravity_east:
        return {-1 * displacement.dx, -0.5 * displacement.dy};

    case mir_placement_gravity_southwest:
        return {0, -1 * displacement.dy};

    case mir_placement_gravity_south:
        return {-0.5 * displacement.dx, -1 * displacement.dy};

    case mir_placement_gravity_southeast:
        return {-1 * displacement.dx, -1 * displacement.dy};

    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("bad placement gravity"));
    }
}
}

auto miral::BasicWindowManager::place_relative(mir::geometry::Rectangle const& parent, WindowSpecification const& parameters, Size size)
-> mir::optional_value<Rectangle>
{
    auto const hints = parameters.placement_hints().value();
    auto const win_gravity = parameters.window_placement_gravity().value();

    if (parameters.size().is_set())
        size = parameters.size().value();

    auto offset = parameters.aux_rect_placement_offset().is_set() ?
                  parameters.aux_rect_placement_offset().value() : Displacement{};

    Rectangle aux_rect = parameters.aux_rect().value();
    aux_rect.top_left = aux_rect.top_left + (parent.top_left-Point{});

    auto const probable_display_area = display_area_for(aux_rect);
    auto const placement_bounds = probable_display_area ? probable_display_area.value()->area : parent;

    std::vector<MirPlacementGravity> rect_gravities{parameters.aux_rect_placement_gravity().value()};

    if (hints & mir_placement_hints_antipodes)
        rect_gravities.push_back(antipodes(parameters.aux_rect_placement_gravity().value()));

    mir::optional_value<Rectangle> default_result;

    for (auto const& rect_gravity : rect_gravities)
    {
        {
            auto result = constrain_to(parent, anchor_for(aux_rect, rect_gravity) + offset) +
                offset_for(size, win_gravity);

            if (placement_bounds.contains(Rectangle{result, size}))
                return Rectangle{result, size};

            if (!default_result.is_set())
                default_result = Rectangle{result, size};
        }

        if (hints & mir_placement_hints_flip_x)
        {
            auto result = constrain_to(parent, anchor_for(aux_rect, flip_x(rect_gravity)) + flip_x(offset)) +
                offset_for(size, flip_x(win_gravity));

            if (placement_bounds.contains(Rectangle{result, size}))
                return Rectangle{result, size};
        }

        if (hints & mir_placement_hints_flip_y)
        {
            auto result = constrain_to(parent, anchor_for(aux_rect, flip_y(rect_gravity)) + flip_y(offset)) +
                offset_for(size, flip_y(win_gravity));

            if (placement_bounds.contains(Rectangle{result, size}))
                return Rectangle{result, size};
        }

        if (hints & mir_placement_hints_flip_x && hints & mir_placement_hints_flip_y)
        {
            auto result = constrain_to(parent, anchor_for(aux_rect, flip_x(flip_y(rect_gravity))) + flip_x(flip_y(offset))) +
                offset_for(size, flip_x(flip_y(win_gravity)));

            if (placement_bounds.contains(Rectangle{result, size}))
                return Rectangle{result, size};
        }
    }

    for (auto const& rect_gravity : rect_gravities)
    {
        auto result = constrain_to(parent, anchor_for(aux_rect, rect_gravity) + offset) +
            offset_for(size, win_gravity);

        if (hints & mir_placement_hints_slide_x)
        {
            auto const left_overhang  = result.x - placement_bounds.top_left.x;
            auto const right_overhang = (result + as_displacement(size)).x - placement_bounds.top_right().x;

            if (left_overhang < DeltaX{0})
                result -= left_overhang;
            else if (right_overhang > DeltaX{0})
                result -= right_overhang;
        }

        if (hints & mir_placement_hints_slide_y)
        {
            auto const top_overhang  = result.y - placement_bounds.top_left.y;
            auto const bot_overhang = (result + as_displacement(size)).y - placement_bounds.bottom_left().y;

            if (top_overhang < DeltaY{0})
                result -= top_overhang;
            else if (bot_overhang > DeltaY{0})
                result -= bot_overhang;
        }

        if (placement_bounds.contains(Rectangle{result, size}))
            return Rectangle{result, size};
    }

    for (auto const& rect_gravity : rect_gravities)
    {
        auto result = constrain_to(parent, anchor_for(aux_rect, rect_gravity) + offset) +
            offset_for(size, win_gravity);

        if (hints & mir_placement_hints_resize_x)
        {
            auto const left_overhang  = result.x - placement_bounds.top_left.x;
            auto const right_overhang = (result + as_displacement(size)).x - placement_bounds.top_right().x;

            if (left_overhang < DeltaX{0})
            {
                result -= left_overhang;
                size = Size{size.width + left_overhang, size.height};
            }

            if (right_overhang > DeltaX{0})
            {
                size = Size{size.width - right_overhang, size.height};
            }
        }

        if (hints & mir_placement_hints_resize_y)
        {
            auto const top_overhang  = result.y - placement_bounds.top_left.y;
            auto const bot_overhang = (result + as_displacement(size)).y - placement_bounds.bottom_left().y;

            if (top_overhang < DeltaY{0})
            {
                result -= top_overhang;
                size = Size{size.width, size.height + top_overhang};
            }

            if (bot_overhang > DeltaY{0})
            {
                size = Size{size.width, size.height - bot_overhang};
            }
        }

        if (placement_bounds.contains(Rectangle{result, size}))
            return Rectangle{result, size};
    }

    return default_result;
}

void miral::BasicWindowManager::validate_modification_request(WindowSpecification const& modifications, WindowInfo const& window_info) const
{
    auto target_type = window_info.type();

    if (modifications.type().is_set())
    {
        auto const original_type = target_type;

        target_type = modifications.type().value();

        switch (original_type)
        {
        case mir_window_type_normal:
        case mir_window_type_utility:
        case mir_window_type_dialog:
        case mir_window_type_satellite:
            switch (target_type)
            {
            case mir_window_type_normal:
            case mir_window_type_utility:
            case mir_window_type_dialog:
            case mir_window_type_satellite:
                break;

            default:
                BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface type change"));
            }
	    // Falls through.

        case mir_window_type_menu:
            switch (target_type)
            {
            case mir_window_type_menu:
            case mir_window_type_satellite:
                break;

            default:
                BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface type change"));
            }
	    // Falls through.

        case mir_window_type_gloss:
        case mir_window_type_freestyle:
        case mir_window_type_inputmethod:
        case mir_window_type_tip:
        case mir_window_type_decoration:
            if (target_type != original_type)
                BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface type change"));
            break;

        default:
            break;
        }
    }

    switch (target_type)
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
        if (modifications.parent().is_set() ? modifications.parent().value().lock() : window_info.parent())
            BOOST_THROW_EXCEPTION(std::runtime_error("Window type must not have a parent"));
        break;

    case mir_window_type_satellite:
    case mir_window_type_gloss:
    case mir_window_type_tip:
        if (modifications.parent().is_set() ? !modifications.parent().value().lock() : !window_info.parent())
            BOOST_THROW_EXCEPTION(std::runtime_error("Window type must have a parent"));
        break;

    case mir_window_type_inputmethod:
    case mir_window_type_dialog:
    case mir_window_type_menu:
    case mir_window_type_freestyle:
    case mir_window_type_decoration:
        break;

    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface type"));
    }

    if (modifications.size().is_set())
    {
        auto const size = modifications.size().value();

        if (size.width <= Width{0})
            BOOST_THROW_EXCEPTION(std::runtime_error("width must be positive"));

        if (size.height <= Height{0})
            BOOST_THROW_EXCEPTION(std::runtime_error("height must be positive"));
    }
}

class miral::Workspace
{
public:
    explicit Workspace(std::shared_ptr<miral::BasicWindowManager::DeadWorkspaces> const& dead_workspaces) :
        dead_workspaces{dead_workspaces} {}

    std::weak_ptr<Workspace> self;

    ~Workspace()
    {
        std::lock_guard lock {dead_workspaces->dead_workspaces_mutex};
        dead_workspaces->workspaces.push_back(self);
    }

private:
    std::shared_ptr<miral::BasicWindowManager::DeadWorkspaces> const dead_workspaces;
};

auto miral::BasicWindowManager::create_workspace() -> std::shared_ptr<Workspace>
{
    auto const result = std::make_shared<Workspace>(dead_workspaces);
    result->self = result;
    return result;
}

void miral::BasicWindowManager::add_tree_to_workspace(
    miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace)
{
    if (!window) return;

    auto root = window;
    auto const* info = &info_for(root);

    while (auto const& parent = info->parent())
    {
        root = parent;
        info = &info_for(root);
    }

    std::vector<Window> windows;

    std::function<void(WindowInfo const& info)> const add_children =
        [&,this](WindowInfo const& info)
        {
            for (auto const& child : info.children())
            {
                windows.push_back(child);
                add_children(info_for(child));
            }
        };

    windows.push_back(root);
    add_children(*info);

    auto const iter_pair = workspaces_to_windows.left.equal_range(workspace);

    std::vector<Window> windows_added;

    for (auto& w : windows)
    {
        if (!std::count_if(iter_pair.first, iter_pair.second,
                           [&w](wwbimap_t::left_value_type const& kv) { return kv.second == w; }))
        {
            workspaces_to_windows.left.insert(wwbimap_t::left_value_type{workspace, w});
            windows_added.push_back(w);
        }
    }

    if (!windows_added.empty())
        policy->advise_adding_to_workspace(workspace, windows_added);
}

void miral::BasicWindowManager::remove_tree_from_workspace(
    miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace)
{
    if (!window) return;

    auto root = window;
    auto const* info = &info_for(root);

    while (auto const& parent = info->parent())
    {
        root = parent;
        info = &info_for(root);
    }

    std::vector<Window> windows;

    std::function<void(WindowInfo const& info)> const add_children =
        [&,this](WindowInfo const& info)
        {
            for (auto const& child : info.children())
            {
                windows.push_back(child);
                add_children(info_for(child));
            }
        };

    windows.push_back(root);
    add_children(*info);

    std::vector<Window> windows_removed;

    auto const iter_pair = workspaces_to_windows.left.equal_range(workspace);
    for (auto kv = iter_pair.first; kv != iter_pair.second;)
    {
        auto const current = kv++;
        if (std::count(begin(windows), end(windows), current->second))
        {
            windows_removed.push_back(current->second);
            workspaces_to_windows.left.erase(current);
        }
    }

    if (!windows_removed.empty())
        policy->advise_removing_from_workspace(workspace, windows_removed);
}

void miral::BasicWindowManager::move_workspace_content_to_workspace(
    std::shared_ptr<Workspace> const& to_workspace, std::shared_ptr<Workspace> const& from_workspace)
{
    std::vector<Window> windows_removed;

    auto const iter_pair_from = workspaces_to_windows.left.equal_range(from_workspace);
    for (auto kv = iter_pair_from.first; kv != iter_pair_from.second;)
    {
        auto const current = kv++;
        windows_removed.push_back(current->second);
        workspaces_to_windows.left.erase(current);
    }

    if (!windows_removed.empty())
        policy->advise_removing_from_workspace(from_workspace, windows_removed);

    std::vector<Window> windows_added;

    auto const iter_pair_to = workspaces_to_windows.left.equal_range(to_workspace);
    for (auto& w : windows_removed)
    {
        if (!std::count_if(iter_pair_to.first, iter_pair_to.second,
                           [&w](wwbimap_t::left_value_type const& kv) { return kv.second == w; }))
        {
            workspaces_to_windows.left.insert(wwbimap_t::left_value_type{to_workspace, w});
            windows_added.push_back(w);
        }
    }

    if (!windows_added.empty())
        policy->advise_adding_to_workspace(to_workspace, windows_added);
}

void miral::BasicWindowManager::for_each_workspace_containing(
    miral::Window const& window, std::function<void(std::shared_ptr<miral::Workspace> const&)> const& callback)
{
    auto const iter_pair = workspaces_to_windows.right.equal_range(window);
    for (auto kv = iter_pair.first; kv != iter_pair.second; ++kv)
    {
        if (auto const workspace = kv->second.lock())
            callback(workspace);
    }
}

void miral::BasicWindowManager::for_each_window_in_workspace(
    std::shared_ptr<miral::Workspace> const& workspace, std::function<void(miral::Window const&)> const& callback)
{
    auto const iter_pair = workspaces_to_windows.left.equal_range(workspace);
    for (auto kv = iter_pair.first; kv != iter_pair.second; ++kv)
        callback(kv->second);
}

auto miral::BasicWindowManager::apply_exclusive_rect_to_application_zone(
    Rectangle const& original_zone,
    Rectangle const& exclusive_rect,
    MirPlacementGravity edges) -> Rectangle
{
    /// if attached to both left and right, we can ignore both
    if ((edges & mir_placement_gravity_west) && (edges & mir_placement_gravity_east))
    {
        edges = (MirPlacementGravity)(edges & ~mir_placement_gravity_west);
        edges = (MirPlacementGravity)(edges & ~mir_placement_gravity_east);
    }

    /// If attached to both top and bottom, we can ignore both
    if ((edges & mir_placement_gravity_north) && (edges & mir_placement_gravity_south))
    {
        edges = (MirPlacementGravity)(edges & ~mir_placement_gravity_north);
        edges = (MirPlacementGravity)(edges & ~mir_placement_gravity_south);
    }

    Rectangle zone{original_zone};

    /// Now that we got rid of stretched attachments, the only states we care about are the following four:
    switch (edges)
    {
    case mir_placement_gravity_west:
        zone.size.width = as_width(zone.right() - as_delta(exclusive_rect.right()));
        zone.top_left.x = exclusive_rect.right();
        break;

    case mir_placement_gravity_east:
        zone.size.width = as_width(exclusive_rect.left() - as_delta(zone.left()));
        break;

    case mir_placement_gravity_north:
        zone.size.height = as_height(zone.bottom() - as_delta(exclusive_rect.bottom()));
        zone.top_left.y = exclusive_rect.bottom();
        break;

    case mir_placement_gravity_south:
        zone.size.height = as_height(exclusive_rect.top() - as_delta(zone.top()));
        break;

    default:
        break;
    }

    return zone;
}

auto miral::BasicWindowManager::add_output_to_display_areas(Locker const&, Output const& output)
{
    std::shared_ptr<DisplayArea> existing_area;

    if (output.logical_group_id())
    {
        auto const area_iter = std::find_if(display_areas.begin(), display_areas.end(), [&](auto const& area)
            {
                return (
                    area->logical_output_group_id &&
                    area->logical_output_group_id.value() == output.logical_group_id());
            });
        if (area_iter != display_areas.end())
        {
            existing_area = *area_iter;
        }
    }

    if (existing_area)
    {
        existing_area->contained_outputs.push_back(output);
        existing_area->area = existing_area->bounding_rectangle_of_contained_outputs();
    }
    else
    {
        auto const new_area = std::make_shared<DisplayArea>(output);
        if (output.logical_group_id())
        {
            new_area->logical_output_group_id = output.logical_group_id();
        }
        display_areas.push_back(new_area);
    }
}

auto miral::BasicWindowManager::remove_output_from_display_areas(Locker const&, Output const& output)
{
    // Remove the output from all areas that contain it
    for (auto const& area : display_areas)
    {
        auto const removed = std::remove_if(
            area->contained_outputs.begin(),
            area->contained_outputs.end(),
            [&](Output const& area_output)
            {
                return area_output.is_same_output(output);
            });
        bool const output_removed{removed != area->contained_outputs.end()};
        area->contained_outputs.erase(removed, area->contained_outputs.end());

        if (output_removed)
        {
            area->area = area->bounding_rectangle_of_contained_outputs();
        }
    }
}

void miral::BasicWindowManager::advise_output_create(miral::Output const& output)
{
    Locker lock{this};

    outputs.add(output.extents());

    add_output_to_display_areas(lock, output);
    application_zones_need_update = true;
    policy->advise_output_create(output);
}

void miral::BasicWindowManager::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    Locker lock{this};

    outputs.remove(original.extents());
    outputs.add(updated.extents());

    if (original.logical_group_id() == updated.logical_group_id())
    {
        for (auto& area : display_areas)
        {
            bool update_extents{false};
            for (auto& output : area->contained_outputs)
            {
                if (output.is_same_output(original))
                {
                    output = updated;
                    update_extents = true;
                }
            }
            if (update_extents)
            {
                area->area = area->bounding_rectangle_of_contained_outputs();
            }
        }
    }
    else
    {
        remove_output_from_display_areas(lock, original);
        add_output_to_display_areas(lock, updated);
    }

    application_zones_need_update = true;
    policy->advise_output_update(updated, original);
}

void miral::BasicWindowManager::advise_output_delete(miral::Output const& output)
{
    Locker lock{this};

    outputs.remove(output.extents());

    remove_output_from_display_areas(lock, output);
    application_zones_need_update = true;
    policy->advise_output_delete(output);
}

void miral::BasicWindowManager::advise_output_end()
{
    Locker lock{this};

    if (application_zones_need_update)
    {
        update_application_zones_and_attached_windows();
        application_zones_need_update = false;
    }
}

void miral::BasicWindowManager::update_application_zones_and_attached_windows()
{
    // Move all live areas to before the split and areas that should be removed to after
    auto split = std::stable_partition(
        display_areas.begin(),
        display_areas.end(),
        [&](std::shared_ptr<DisplayArea> const& area)
        {
            return area->is_alive();
        });

    // Move areas after the split into a new vector
    std::vector<std::shared_ptr<DisplayArea>> removed_areas;
    std::move(split, display_areas.end(), std::back_inserter(removed_areas));

    // Clean up empty slots left behind
    display_areas.erase(split, display_areas.end());

    for (auto const& area : removed_areas)
    {
        // Move windows that were attached to the removed areas
        for (auto const& window : area->attached_windows)
        {
            auto info{info_for(window)};
            update_attached_and_fullscreen_sets(info);
        }

        // Tell the policy about removed application zones
        if (area->zone_policy_knows_about)
        {
            policy->advise_application_zone_delete(area->zone_policy_knows_about.value());
        }
    }

    // Fullscreen surface should fill the whole area (does not depend on what the zones end up being)
    for (auto const& window : fullscreen_surfaces)
    {
        if (window)
        {
            auto& info = info_for(window);
            auto const rect =
                policy->confirm_placement_on_display(info, mir_window_state_fullscreen, display_area_for(info)->area);
            place_and_size(info, rect.top_left, rect.size);
        }
    }

    for (auto& area : display_areas)
    {
        Rectangle zone_rect = area->area;

        /// The first pass will modify the application zone as it goes
        /// The second pass will use the final application zone
        std::vector<WindowInfo*> first_pass;
        std::vector<WindowInfo*> second_pass;

        for (auto const& window : area->attached_windows)
        {
            if (window)
            {
                auto& info = info_for(window);

                switch (info.state())
                {
                case mir_window_state_attached:
                    if (info.exclusive_rect().is_set())
                    {
                        first_pass.push_back(&info);
                        break;
                    }
                    // fallthrough
                case mir_window_state_maximized:
                case mir_window_state_horizmaximized:
                case mir_window_state_vertmaximized:
                    second_pass.push_back(&info);
                    break;

                default:
                    log_error("Window in attached_windows is not attached or maximized");
                    break;
                }
            }
        }

        for (auto info_ptr : first_pass)
        {
            place_attached_to_zone(*info_ptr, zone_rect);

            auto& info = *info_ptr;
            Rectangle exclusive_rect{
                info.exclusive_rect().value().top_left + as_displacement(info.window().top_left()),
                info.exclusive_rect().value().size};

            zone_rect = apply_exclusive_rect_to_application_zone(zone_rect, exclusive_rect, info.attached_edges());
        }

        for (auto info_ptr : second_pass)
        {
            place_attached_to_zone(*info_ptr, zone_rect);
        }

        area->application_zone.extents(zone_rect);
        if (!area->zone_policy_knows_about)
        {
            policy->advise_application_zone_create(area->application_zone);
        }
        if (area->zone_policy_knows_about && !(area->application_zone == area->zone_policy_knows_about.value()))
        {
            policy->advise_application_zone_update(area->application_zone, area->zone_policy_knows_about.value());
        }
        area->zone_policy_knows_about = area->application_zone;
    }
}
