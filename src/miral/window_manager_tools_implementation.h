/*
 * Copyright © 2016-2020 Canonical Ltd.
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

#ifndef MIRAL_WINDOW_MANAGER_TOOLS_IMPLEMENTATION_H
#define MIRAL_WINDOW_MANAGER_TOOLS_IMPLEMENTATION_H

#include "miral/application.h"

#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangle.h>

#include <functional>
#include <memory>
#include <vector>

namespace mir { namespace scene { class Surface; } }

namespace miral
{
class Window;
struct WindowInfo;
struct ApplicationInfo;
class WindowSpecification;
class Workspace;

// The interface through which the policy instructs the controller.
class WindowManagerToolsImplementation
{
public:
/** @name Update Model
 *  These functions assume that the BasicWindowManager data structures can be accessed freely.
 *  I.e. they should only be used by a thread that has called the WindowManagementPolicy methods
 *  (where any necessary locks are held) or via a invoke_under_lock() callback.
 *  @{ */
    virtual auto count_applications() const -> unsigned int = 0;
    virtual void for_each_application(std::function<void(ApplicationInfo& info)> const& functor) = 0;
    virtual auto find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
    -> Application = 0;
    virtual auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo& = 0;
    virtual auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo& = 0;
    virtual auto info_for(Window const& window) const -> WindowInfo& = 0;

    virtual void ask_client_to_close(Window const& window) = 0;
    virtual auto active_window() const -> Window = 0;
    virtual auto select_active_window(Window const& hint) -> Window = 0;
    virtual void drag_active_window(mir::geometry::Displacement movement) = 0;
    virtual void drag_window(Window const& window, mir::geometry::Displacement& movement) = 0;
    virtual void focus_next_application() = 0;
    virtual void focus_prev_application() = 0;
    virtual void focus_next_within_application() = 0;
    virtual void focus_prev_within_application() = 0;
    virtual auto window_at(mir::geometry::Point cursor) const -> Window = 0;
    virtual auto active_output() -> mir::geometry::Rectangle const = 0;
    virtual void raise_tree(Window const& root) = 0;
    virtual void start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle) = 0;
    virtual void end_drag_and_drop() = 0;
    virtual void modify_window(WindowInfo& window_info, WindowSpecification const& modifications) = 0;
    virtual auto info_for_window_id(std::string const& id) const -> WindowInfo& = 0;
    virtual auto id_for_window(Window const& window) const -> std::string = 0;
    virtual void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const= 0;

    virtual auto create_workspace() -> std::shared_ptr<Workspace> = 0;
    virtual void add_tree_to_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace) = 0;
    virtual void remove_tree_from_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace) = 0;
    virtual void move_workspace_content_to_workspace(
        std::shared_ptr<Workspace> const& to_workspace,
        std::shared_ptr<Workspace> const& from_workspace) = 0;
    virtual void for_each_workspace_containing(
        Window const& window,
        std::function<void(std::shared_ptr<Workspace> const& workspace)> const& callback) = 0;
    virtual void for_each_window_in_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::function<void(Window const& window)> const& callback) = 0;

/** @} */

/** @name Multi-thread support
 *  Allows threads that don't hold a lock on the model to acquire one and call the "Update Model"
 *  member functions.
 *  This should NOT be used by a thread that has called the WindowManagementPolicy methods (and
 *  already holds the lock).
 *  @{ */
    virtual void invoke_under_lock(std::function<void()> const& callback) = 0;
/** @} */

    virtual ~WindowManagerToolsImplementation() = default;
    WindowManagerToolsImplementation() = default;
    WindowManagerToolsImplementation(WindowManagerToolsImplementation const&) = delete;
    WindowManagerToolsImplementation& operator=(WindowManagerToolsImplementation const&) = delete;
};
}

#endif //MIRAL_WINDOW_MANAGER_TOOLS_IMPLEMENTATION_H
