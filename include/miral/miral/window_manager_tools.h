/*
 * Copyright © Canonical Ltd.
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

#ifndef MIRAL_WINDOW_MANAGER_TOOLS_H
#define MIRAL_WINDOW_MANAGER_TOOLS_H

#include "miral/application.h"
#include "window_info.h"

#include <mir/geometry/displacement.h>

#include <functional>
#include <memory>
#include <optional>

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
class Zone;

class Workspace;

class WindowManagerToolsImplementation;

/// This class provides compositor authors with facilities to query state info for and
/// request changes to elements of the window management model (such as windows
/// and outputs).
///
/// When a #WindowManagementPolicy is added to a #miral::MirRunner via #miral::add_window_manager_policy,
/// the policy will receive an instance of #miral::WindowManagerTools as the first parameter.
/// Compositor authors are encouraged to use this class instance to handle and request changes from
/// the underlying Mir server.
class WindowManagerTools
{
public:
    /// Constructs an instance of window manager tools from an implementation.
    ///
    /// Compositor authors are not expected to construct this object themselves.
    /// Instead, they should use #miral::add_window_manager_policy to create
    /// an instance of this class for them. For this reason,
    /// #miral::WindowManagerToolsImplementation is purposefully opaque.
    explicit WindowManagerTools(WindowManagerToolsImplementation* tools);
    WindowManagerTools(WindowManagerTools const&);
    WindowManagerTools& operator=(WindowManagerTools const&);
    ~WindowManagerTools();

    /// Returns the number of applications.
    ///
    /// @returns the number of applications
    auto count_applications() const -> unsigned int;

    /// Execute a functor for each application.
    ///
    /// \param functor called for each application
    void for_each_application(std::function<void(ApplicationInfo& info)> const& functor);

    /** find an application meeting the predicate
     *
     * @param predicate the predicate
     * @return          the application
     */
    /// Find an application that meets the predicate.
    ///
    /// \param predicate the predicate
    /// \returns an application, or a null application if none is found
    auto find_application(std::function<bool(ApplicationInfo const& info)> const& predicate)
    -> Application;

    /// Retrieve information about a \p session.
    ///
    /// \param session the application
    /// \returns info about the application
    /// \pre the \p session is valid (not null)
    /// \sa miral::ApplicationInfo - info about the application
    auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> ApplicationInfo&;

    /// Retrieve info about a window from the \p surface.
    ///
    /// \param surface the surface
    /// \returns info about the window
    /// \pre the \p session is valid (not null)
    /// \sa miral::WindowInfo - info about the window
    auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> WindowInfo&;

    /// Retrieve info about a window.
    ///
    /// \param window the window
    /// \returns info about the window
    /// \pre the \p session is valid (not null)
    /// \sa miral::WindowInfo - info about the window
    auto info_for(Window const& window) const -> WindowInfo&;

    /// Retrieve info for a persistent surface id.
    ///
    /// \param id the persistent surface id
    /// \returns the info
    /// \throws invalid_argument if the id is badly formatted
    /// \throws runtime_error if the id doesn't identify a current window
    /// \deprecated 'Persistent' surface IDs were part of mirclient API
    [[deprecated("'Persistent' surface IDs were part of mirclient API")]]
    auto info_for_window_id(std::string const& id) const -> WindowInfo&;

    /// Retrieve the persistent surface id for a window
    ///
    /// \param window the window
    /// \returns the persistent surface id
    /// \deprecated 'Persistent' surface IDs were part of mirclient API
    [[deprecated("'Persistent' surface IDs were part of mirclient API")]]
    auto id_for_window(Window const& window) const -> std::string;

    /// Send a close request to the window.
    ///
    /// \param window the window
    void ask_client_to_close(Window const& window);

    /// Retrieve the active window.
    ///
    /// If no window is active, then this will be the default-constructed window.
    ///
    /// \returns the active window
    auto active_window() const -> Window;

    /// Select a new active window based on the \p hint.
    /// 
    /// Not all windows can become active - either because of their type
    /// or other state. But there will typically be an associated parent or
    /// child window that can become active.
    ///
    /// \param hint the window hint
    /// \returns the new active window
    auto select_active_window(Window const& hint) -> Window;

    /// Drags the active window by the provided displacement.
    ///
    /// \param movement the displacement to move by
    void drag_active_window(mir::geometry::Displacement movement);

    /// Drag the provided \p window by the provided displacement.
    ///
    /// \param window the window to move
    /// \param movement the displacement to move by
    void drag_window(Window const& window, mir::geometry::Displacement movement);

    /// Make the next application in the focus order after the currently
    /// focused application active.
    ///
    /// "Next" is defined as the next most-recently focused application.
    void focus_next_application();

    /// Make the previous application in the focus order after the currently
    /// focused application active.
    ///
    /// "Previous" is defined as the least-recently focused application.
    void focus_prev_application();

    /// Focus the next window within the currently active application.
    ///
    /// "Next" is defined as the next most-recently focused window.
    void focus_next_within_application();

    /// Focus the previous window within the currently active application.
    ///
    /// "Previous" is defined as the least-recently focused window.
    void focus_prev_within_application();

    /// Returns the window of the given application that should be selected.
    ///
    /// \returns the window to select, or `std::nullopt` if none are found
    auto window_to_select_application(const Application) const -> std::optional<Window>;

    /// Check if the provided \p window can be selected.
    ///
    /// \param window the window
    /// \returns `true` if the window can be selected, otherwise `false`
    /// \remark Since MirAL 5.0
    auto can_select_window(Window const& window) const -> bool;

    /// Find the topmost window at the cursor.
    ///
    /// \param cursor a point
    /// \returns the window under the cursor, or the default-constructed window if none exists
    auto window_at(mir::geometry::Point cursor) const -> Window;

    /// Find the area of the active output.
    ///
    /// \returns the area of the active output
    auto active_output() -> mir::geometry::Rectangle const;

    /// Find the active zone area.
    ///
    /// \returns the active zone
    auto active_application_zone() const -> Zone;

    /// Raise the provided window and its children to the top of the Z-order.
    ///
    /// \param root the window to raise
    void raise_tree(Window const& root);

    /// Swaps the position of the windows in regard to Z-order.
    ///
    /// \param first the first window
    /// \param second the second window
    void swap_tree_order(Window const& first, Window const& second);

    /// Moves the window to the bottom of the Z order.
    ///
    /// \param root the window to send to the back of the Z order
    void send_tree_to_back(Window const& root);

    /// Modify the provided window with the provided \p modifications.
    ///
    /// \param window_info the window to modify
    /// \param modifications the modification sto make on the window
    void modify_window(WindowInfo& window_info, WindowSpecification const& modifications);

    /// Modify the provided \p window with the provided \p modifications.
    ///
    /// \param window the window to modify
    /// \param modifications the modification sto make on the window
    void modify_window(Window const& window, WindowSpecification const& modifications);

    /// Set a default size and position to reflect state change.
    ///
    /// This method has no effect when #WindowSpecification::state is unset.
    ///
    /// \param modifications modifications, with state() set
    /// \param window_info the window to modify
    void place_and_size_for_state(WindowSpecification& modifications, WindowInfo const& window_info) const;

    /// Create a workspace.
    ///
    /// \returns the created workspace
    auto create_workspace() -> std::shared_ptr<Workspace>;

    /// Add the \p window to the \p workspace.
    ///
    /// A window may be added to more than one workspace.
    ///
    /// The #miral::Workspace is a purely associative contract. It is the job of the
    /// compositor author to define what this means for their particular use case.
    /// For example, the author may make it such that a particular workspace be
    /// "active" and only windows on the active workspace are shown.
    ///
    /// \param window the window
    /// \param workspace the workspace
    void add_tree_to_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace);

    /// Remove the \p window from the \p workspace.
    ///
    /// \param window the window
    /// \param workspace the workspace
    void remove_tree_from_workspace(Window const& window, std::shared_ptr<Workspace> const& workspace);

    /// Move the windows associated with one workspace to another.
    ///
    /// \param to_workspace  the workspace to move windows to
    /// \param from_workspace the workspace to move windows from
    void move_workspace_content_to_workspace(
        std::shared_ptr<Workspace> const& to_workspace,
        std::shared_ptr<Workspace> const& from_workspace);


    /// Invoke the \p callback for each workspace that contains the \p window.
    ///
    /// \warning It is unsafe to add or remove windows from workspace from the callback during enumeration.
    /// \param window the window
    /// \param callback called for each workspace that the window is a member of
    void for_each_workspace_containing(
        Window const& window,
        std::function<void(std::shared_ptr<Workspace> const& workspace)> const& callback);

    /// Invoke the \p callback on each window contained in the \p workspace.
    ///
    /// \warning It is unsafe to add or remove windows from workspace from the callback during enumeration.
    /// \param workspace the workspace
    /// \param callback called for each window in the workspace
    void for_each_window_in_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::function<void(Window const& window)> const& callback);

    /// Invoke the given \p callback under the lock
    ///
    /// This method allows threads that do not hold a lock on the model to acquire one and
    /// call any relevant member functions.
    ///
    /// This should NOT be used by a thread that has called the #miral::WindowManagementPolicy methods,
    /// as they already hold the lock.
    ///
    /// \param callback the method to call under lock
    void invoke_under_lock(std::function<void()> const& callback);

    /// Move the cursor to the provided \point.
    ///
    /// If the point is beyond the range of the outputs, the point is clamped to the output area.
    /// \param point to move the cursor to
    void move_cursor_to(mir::geometry::PointF point);

private:
    WindowManagerToolsImplementation* tools;
};
}

#endif //MIRAL_WINDOW_MANAGER_TOOLS_H
