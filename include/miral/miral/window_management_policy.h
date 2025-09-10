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

#ifndef MIRAL_WINDOW_MANAGEMENT_POLICY_H
#define MIRAL_WINDOW_MANAGEMENT_POLICY_H

#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangles.h>
#include <mir_toolkit/common.h>

struct MirKeyboardEvent;
struct MirTouchEvent;
struct MirPointerEvent;
struct MirInputEvent;

#include <memory>

namespace miral
{
class Window;
class WindowSpecification;
struct ApplicationInfo;
class Output;
class Zone;
struct WindowInfo;


/// An opaque workspace indentifier.
///
/// This symbol is purposefully opaque because it is only ever provided as a
/// `std::shared_ptr` which is used as an unique identifier.
class Workspace;

using namespace mir::geometry;

/// An interface that defines the window management policy for the compositor.
///
/// Compositor authors may implement this interface to provide custom
/// behavior in response to events in the compositor. For example, the
/// author may override #WindowManagementPolicy::advise_new_window to perform some
/// action when a new window becomes available.
///
/// Instances of this class are often used in conjunction with #miral::add_window_manager_policy.
///
/// Methods that are prefixed with "advise_*" are notifications that originate from  the compositor.
/// The policy does not have to take any action in response to these notifications.
///
/// Methods that are prefixed with "handle_*" are requests that originate from the client applications.
/// The policy is expected to take some action in response to these requests.
///
/// \sa miral::add_window_manager_policy - provides a way to add a window management policy to the server
/// \sa miral::WindowManagerOptions - provides a way to add and control multiple window management policies for the server
/// \sa miral::MinimalWindowManager - an implementation of the WindowManagementPolicy that serves as a strong
///                                   foundation for a floating window manager
/// \sa miral::WindowManagerTools - tools provided to the policy via miral::add_window_manager_policy
class WindowManagementPolicy
{
public:
    /// Called before any "advise_*" method is called.
    virtual void advise_begin();

    /// Called after any "advise_*" method is called.
    virtual void advise_end();

    /// Given the \p app_info and \p requested_specification, this method returns
    /// a new #miral::WindowSpecification that defines how the new window should be placed.
    ///
    /// This method is called before #advise_new_window.
    ///
    /// @param app_info the application requesting a new window
    /// @param requested_specification the requested specification with a default position and size
    /// @returns the customized placement
    virtual auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification = 0;

    /// Notification that the first buffer for this window has been posted.
    ///
    /// @param window_info the window that posted the buffer
    virtual void handle_window_ready(WindowInfo& window_info) = 0;

    /// Request from the client that the window given by \p window_info should be modified with
    /// the \p modification.
    ///
    /// The compositor author should use #miral::WindowManagerTools::modify_window to make the appropriate
    /// changes. The compositor may choose to honor or change any of the requested modifications.
    ///
    /// \param window_info the window
    /// \param modifications the requested changes
    /// \sa miral::WindowManagerTools::modify_window - the method used to modify a window
    virtual void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) = 0;

    /** request from client to raise the window
     * \note the request has already been validated against the requesting event
     *
     * @param window_info   the window
     */
    /// Request from the client to raise the window.
    ///
    /// The compositor author may use #miral::WindowManagerTools::raise_tree in response,
    /// or they may choose to do anything else.
    ///
    /// \param window_info the window
    /// \sa miral::WindowManagerTools::raise_tree - the method used to raise a window
    virtual void handle_raise_window(WindowInfo& window_info) = 0;

    /// Confirm and optionally adjust the placement of a window on the display.
    ///
    /// This is called when placing a fullscreen, maximized, horizontally maximized,
    /// and vertically maximized window to allow adjustment for decorations.
    ///
    /// If the author wishes to do nothing in response, they may simply return
    /// \p new_placement.
    ///
    /// \param window_info the window
    /// \param new_state the new state
    /// \param new_placement the suggested placement
    /// \returns the final placement
    virtual auto confirm_placement_on_display(
        WindowInfo const& window_info,
        MirWindowState new_state,
        Rectangle const& new_placement) -> Rectangle = 0;

    /// Handle a keyboard event originating from the user.
    ///
    /// \param event the keyboard event
    /// \return `true` if the policy consumed the event, otherwise `false`
    virtual bool handle_keyboard_event(MirKeyboardEvent const* event) = 0;

    /// Handle a touch event originating from the user.
    ///
    /// \param event the keyboard event
    /// \return `true` if the policy consumed the event, otherwise `false`
    virtual bool handle_touch_event(MirTouchEvent const* event) = 0;

    /// Handle a pointer event originating from the user.
    ///
    /// \param event the pointer event
    /// \return `true` if the policy consumed the event, otherwise `false`
    virtual bool handle_pointer_event(MirPointerEvent const* event) = 0;

    /// Notification that a new application has connected.
    ///
    /// \param application the application
    virtual void advise_new_app(ApplicationInfo& application);

    /// Notification that an application has disconnected.
    ///
    /// @param application the application
    virtual void advise_delete_app(ApplicationInfo const& application);

    /// Notification that a window has been created.
    ///
    /// \param window_info the window
    virtual void advise_new_window(WindowInfo const& window_info);

    /// Notification that a window has lost focus.
    ///
    /// @param window_info the window
    virtual void advise_focus_lost(WindowInfo const& window_info);

    /// Notification that a window has gained focus.
    ///
    /// @param  window_info the window
    virtual void advise_focus_gained(WindowInfo const& window_info);

    /// Notification that a window is about to change state.
    ///
    /// \param window_info the window
    /// \param state the new state
    virtual void advise_state_change(WindowInfo const& window_info, MirWindowState state);

    /// Notification that a window is about to move.
    ///
    /// \param window_info the window
    /// \param top_left the new position
    virtual void advise_move_to(WindowInfo const& window_info, Point top_left);

    /// Notification that a window is about to resize.
    ///
    /// \param window_info the window
    /// \param new_size the new size
    virtual void advise_resize(WindowInfo const& window_info, Size const& new_size);

    /// Notification that a window is about to be destroyed.
    ///
    /// \param window_info the window
    virtual void advise_delete_window(WindowInfo const& window_info);

    /// Notification that the provided \p windows are being raised to the top.
    ///
    /// These windows are ordered with parents before children and form a single
    /// tree rooted at the first element.
    ///
    /// \param windows the windows being raised
    /// \note The relative Z-order of these windows will be maintained, they will be raised en bloc.
    virtual void advise_raise(std::vector<Window> const& windows);

    /// Notification that windows are being added to a workspace.
    ///
    /// These windows are ordered with parents before children, and
    /// form a single tree rooted at the first element.
    ///
    /// \param workspace the workspace
    /// \param windows the windows
    ///
    /// \sa miral::WindowManagerTools::add_tree_to_workspace - called to add windows to a workspace
    virtual void advise_adding_to_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows);

    /// Notification that windows are being removed from a workspace.
    ///
    /// These windows are ordered with parents before children, and
    /// form a single tree rooted at the first element.
    ///
    /// \param workspace the workspace
    /// \param windows the windows
    ///
    /// \sa miral::WindowManagerTools::remove_tree_from_workspace - called to remove windows from a workspace
    virtual void advise_removing_from_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows);

    /// Request from the client to initiate a move.
    ///
    /// \param window_info the window
    /// \param input_event the requesting event
    virtual void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) = 0;

    /// Request from a client to initiate a resize.
    ///
    /// \param window_info the window
    /// \param input_event the requesting event
    /// \param edge the edge being resized
    /// \sa MirResizeEdge - edge resize options
    virtual void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) = 0;

    /// Notification that an output has been created.
    ///
    /// \param output the new output
    virtual void advise_output_create(Output const& output);

    /// Notification that an output has had one or more of its properties changed.
    ///
    /// \param updated the updated output
    /// \param original the old output
    virtual void advise_output_update(Output const& updated, Output const& original);

    /// Notification that an output has been removed.
    ///
    /// \param output the deleted output
    virtual void advise_output_delete(Output const& output);

    /// When a parent window is moved, this method will be called for each child window so that
    /// the compositor can optionally adjust the target position.
    ///
    /// A default implementation is to simply add the displacement to the current positon
    /// of the window.
    ///
    /// \param window_info the window
    /// \param movement the movement of the parent
    ///
    /// \returns the confirmed placement of the window
    virtual auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle = 0;

    /** @name notification of changes to the current application zones
    * An application zone is the area a maximized application will fill.
    * There is often (but not necessarily) one zone per output.
    * The areas normal applications windows should avoid (such as the areas covered by panels)
    * will not be part of an application zone
    *  @{ */

    /// Notification that an application zone has been created.
    ///
    /// An application zone defines the area in which normal applications should be placed.
    /// For example, an application zone make take up an entire output except for the areas
    /// covered by panels.
    ///
    /// An application zone is often created per output, but this does not have to be the case.
    ///
    /// \param application_zone the new application zone
    virtual void advise_application_zone_create(Zone const& application_zone);

    /// Notification that an application zone has been updated.
    ///
    /// \param updated the new application zone
    /// \param original the old application zone
    virtual void advise_application_zone_update(Zone const& updated, Zone const& original);

    /// Notification that an application zone has been removed.
    ///
    /// \param application_zone the removed zone
    virtual void advise_application_zone_delete(Zone const& application_zone);

    virtual ~WindowManagementPolicy();
    WindowManagementPolicy() = default;
    WindowManagementPolicy(WindowManagementPolicy const&) = delete;
    WindowManagementPolicy& operator=(WindowManagementPolicy const&) = delete;
};

class WindowManagerTools;
}

#endif //MIRAL_WINDOW_MANAGEMENT_POLICY_H
