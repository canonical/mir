/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/window_manager_tools.h"
#include "window_manager_tools_implementation.h"

miral::WindowManagerTools::WindowManagerTools(WindowManagerToolsImplementation* tools) :
    tools{tools}
{
}

miral::WindowManagerTools::WindowManagerTools(WindowManagerTools const&) = default;
miral::WindowManagerTools& miral::WindowManagerTools::operator=(WindowManagerTools const&) = default;
miral::WindowManagerTools::~WindowManagerTools() = default;

auto miral::WindowManagerTools::count_applications() const -> unsigned int
{ return tools->count_applications(); }

void miral::WindowManagerTools::for_each_application(std::function<void(ApplicationInfo& info)> const& functor)
{ tools->for_each_application(functor); }

auto miral::WindowManagerTools::find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
-> Application
{ return tools->find_application(predicate); }

auto miral::WindowManagerTools::info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo&
{ return tools->info_for(session); }

auto miral::WindowManagerTools::info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo&
{ return tools->info_for(surface); }

auto miral::WindowManagerTools::info_for(Window const& window) const -> WindowInfo&
{ return tools->info_for(window); }

void miral::WindowManagerTools::ask_client_to_close(Window const& window)
{ tools->ask_client_to_close(window); }

void miral::WindowManagerTools::force_close(Window const& window)
{ tools->force_close(window); }

auto miral::WindowManagerTools::active_window() const -> Window
{ return tools->active_window(); }

auto miral::WindowManagerTools::select_active_window(Window const& hint) -> Window
{ return tools->select_active_window(hint); }

void miral::WindowManagerTools::drag_active_window(mir::geometry::Displacement movement)
{ tools->drag_active_window(movement); }

void miral::WindowManagerTools::drag_window(Window const& window, mir::geometry::Displacement movement)
{ tools->drag_window(window, movement); }

void miral::WindowManagerTools::focus_next_application()
{ tools->focus_next_application(); }

void miral::WindowManagerTools::focus_next_within_application()
{ tools->focus_next_within_application(); }

void miral::WindowManagerTools::focus_prev_within_application()
{ tools->focus_prev_within_application(); }

auto miral::WindowManagerTools::window_at(mir::geometry::Point cursor) const -> Window
{ return tools->window_at(cursor); }

auto miral::WindowManagerTools::active_output() -> mir::geometry::Rectangle const
{ return tools->active_output(); }

void miral::WindowManagerTools::raise_tree(Window const& root)
{ tools->raise_tree(root); }

void miral::WindowManagerTools::start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle)
{ tools->start_drag_and_drop(window_info, handle); }

void miral::WindowManagerTools::end_drag_and_drop()
{ tools->end_drag_and_drop(); }

void miral::WindowManagerTools::modify_window(WindowInfo& window_info, WindowSpecification const& modifications)
{ tools->modify_window(window_info,modifications); }

void miral::WindowManagerTools::modify_window(Window const& window, WindowSpecification const& modifications)
{ tools->modify_window(tools->info_for(window), modifications); }

auto miral::WindowManagerTools::info_for_window_id(std::string const& id) const -> WindowInfo&
{ return tools->info_for_window_id(id); }

auto miral::WindowManagerTools::id_for_window(Window const& window) const -> std::string
{ return tools->id_for_window(window); }

void miral::WindowManagerTools::invoke_under_lock(std::function<void()> const& callback)
{ tools->invoke_under_lock(callback); }

void miral::WindowManagerTools::place_and_size_for_state(
    WindowSpecification& modifications, WindowInfo const& window_info) const
{ tools->place_and_size_for_state(modifications, window_info); }

auto miral::WindowManagerTools::create_workspace() -> std::shared_ptr<miral::Workspace>
{ return tools->create_workspace(); }

void miral::WindowManagerTools::add_tree_to_workspace(
    miral::Window const& window,
    std::shared_ptr<miral::Workspace> const& workspace)
{ tools->add_tree_to_workspace(window, workspace); }

void miral::WindowManagerTools::remove_tree_from_workspace(
    miral::Window const& window,
    std::shared_ptr<miral::Workspace> const& workspace)
{ tools->remove_tree_from_workspace(window, workspace); }

void miral::WindowManagerTools::move_workspace_content_to_workspace(
    std::shared_ptr<Workspace> const& to_workspace,
    std::shared_ptr<Workspace> const& from_workspace)
{ tools->move_workspace_content_to_workspace(to_workspace, from_workspace); }

void miral::WindowManagerTools::for_each_workspace_containing(
    miral::Window const& window,
    std::function<void(std::shared_ptr<miral::Workspace> const&)> const& callback)
{ tools->for_each_workspace_containing(window, callback); }

void miral::WindowManagerTools::for_each_window_in_workspace(
    std::shared_ptr<miral::Workspace> const& workspace,
    std::function<void(miral::Window const&)> const& callback)
{ tools->for_each_window_in_workspace(workspace, callback); }
