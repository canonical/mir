/*
 * Copyright © 2016-2017 Canonical Ltd.
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

#ifndef MIRAL_WINDOW_MANAGER_TOOLS_H
#define MIRAL_WINDOW_MANAGER_TOOLS_H

#include "miral/application.h"
#include "window_info.h"

#include <mir/geometry/displacement.h>

#include <functional>
#include <memory>

namespace mir
{
namespace scene { class Surface; }
}

namespace miral
{
class Window;
struct WindowInfo;
struct ApplicationInfo;
class WindowSpecification;

/**
 * Workspace is intentionally opaque in the miral API. Its only purpose is to
 * provide a shared_ptr which is used as an identifier.
 *
 * The MirAL implementation of workspaces only prescribes the following:
 *  o When child windows are created they are added to all(any) workspaces of parent
 *  o Focus changes will first try windows with a common workspace
 *  o Adding/removing windows to a workspace affects the whole ancestor/decendent tree
 *
 * The presentation of workspaces is left entirely to the policy
 */
class Workspace;

class WindowManagerToolsImplementation;

/// Window management functions for querying and updating MirAL's model
class WindowManagerTools
{
public:
    explicit WindowManagerTools(WindowManagerToolsImplementation* tools);
    WindowManagerTools(WindowManagerTools const&);
    WindowManagerTools& operator=(WindowManagerTools const&);
    ~WindowManagerTools();

/** @name Query & Update Model
 *  These functions assume that the BasicWindowManager data structures can be accessed freely.
 *  I.e. they should only be used by a thread that has called the WindowManagementPolicy methods
 *  (where any necessary locks are held) or via a invoke_under_lock() callback.
 *  @{ */

    /** count the applications
     *
     * @return number of applications
     */
    auto count_applications() const -> unsigned int;

    /** execute functor for each application
     *
     * @param functor the functor
     */
    void for_each_application(std::function<void(ApplicationInfo& info)> const& functor);

    /** find an application meeting the predicate
     *
     * @param predicate the predicate
     * @return          the application
     */
    auto find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
    -> Application;

    /** retrieve metadata for an application
     *
     * @param session   the application session
     * @return          the metadata
     */
    auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo&;

    /** retrieve metadata for a window
     *
     * @param surface   the window surface
     * @return          the metadata
     */
    auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo&;

    /** retrieve metadata for a window
     *
     * @param window    the window
     * @return          the metadata
     */
    auto info_for(Window const& window) const -> WindowInfo&;

    /** retrieve metadata for a persistent surface id
     *
     * @param id        the persistent surface id
     * @return          the metadata
     * @throw           invalid_argument or runtime_error if the id is badly formatted/doesn't identify a current window
     */
    auto info_for_window_id(std::string const& id) const -> WindowInfo&;

    /** retrieve the persistent surface id for a window
     *
     * @param window    the window
     * @return          the persistent surface id
     */
    auto id_for_window(Window const& window) const -> std::string;

    /// Send close request to the window
    void ask_client_to_close(Window const& window);

    /// Close the window by force
    /// \note ask_client_to_close() is the polite way
    void force_close(Window const& window);

    /// retrieve the active window
    auto active_window() const -> Window;

    /** select a new active window based on the hint
     *
     * @param hint  the hint
     * @return      the new active window
     */
    auto select_active_window(Window const& hint) -> Window;

    /// move the active window
    void drag_active_window(mir::geometry::Displacement movement);

    /// move the window
    void drag_window(Window const& window, mir::geometry::Displacement movement);

    /// make the next application active
    void focus_next_application();

    /// make the previous application active
    /// \remark Since MirAL 2.5
    void focus_prev_application();

    /// make the next surface active within the active application
    void focus_next_within_application();

    /// make the prev surface active within the active application
    void focus_prev_within_application();

    /// Find the topmost window at the cursor
    auto window_at(mir::geometry::Point cursor) const -> Window;

    /// Find the active output area
    auto active_output() -> mir::geometry::Rectangle const;

    /// Raise window and all its children
    void raise_tree(Window const& root);

    /** Start drag and drop. The handle will be passed to the client which can
     * then use it to talk to the whatever service is being used to support drag
     * and drop (e.g. on Ubuntu the content hub).
     *
     * @param window_info source window
     * @param handle      drag handle
     */
    void start_drag_and_drop(WindowInfo& window_info, std::vector<uint8_t> const& handle);

    /// End drag and drop
    void end_drag_and_drop();

    /// Apply modifications to a window
    void modify_window(WindowInfo& window_info, WindowSpecification const& modifications);

    /// Apply modifications to a window
    void modify_window(Window const& window, WindowSpecification const& modifications);

    /// Set a default size and position to reflect state change
    void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const;

    /** Create a workspace.
     * \remark the tools hold only a weak_ptr<> to the workspace - there is no need for an explicit "destroy".
     * @return a shared_ptr owning the workspace
     */
    auto create_workspace() -> std::shared_ptr<Workspace>;

    /**
     * Add the tree containing window to a workspace
     * @param window    the window
     * @param workspace the workspace;
     */
    void add_tree_to_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace);

    /**
     * Remove the tree containing window from a workspace
     * @param window    the window
     * @param workspace the workspace;
     */
    void remove_tree_from_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace);

    /**
     * Moves all the content from one workspace to another
     * @param from_workspace the workspace to move the windows from;
     * @param to_workspace the workspace to move the windows to;
     */
    void move_workspace_content_to_workspace(
        std::shared_ptr<Workspace> const& to_workspace,
        std::shared_ptr<Workspace> const& from_workspace);

    /**
     * invoke callback with each workspace containing window
     * \warning it is unsafe to add or remove windows from workspaces from the callback during enumeration
     * @param window
     * @param callback
     */
    void for_each_workspace_containing(
        Window const& window,
        std::function<void(std::shared_ptr<Workspace> const& workspace)> const& callback);

    /**
     * invoke callback with each window contained in workspace
     * \warning it is unsafe to add or remove windows from workspaces from the callback during enumeration
     * @param workspace
     * @param callback
     */
    void for_each_window_in_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::function<void(Window const& window)> const& callback);

/** @} */

    /** Multi-thread support
     *  Allows threads that don't hold a lock on the model to acquire one and call the "Update Model"
     *  member functions.
     *  This should NOT be used by a thread that has called the WindowManagementPolicy methods (and
     *  already holds the lock).
     */
    void invoke_under_lock(std::function<void()> const& callback);

private:
    WindowManagerToolsImplementation* tools;
};
}

#endif //MIRAL_WINDOW_MANAGER_TOOLS_H
