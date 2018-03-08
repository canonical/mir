/*
 * Copyright © 2016-2018 Canonical Ltd.
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

#ifndef MIRAL_WINDOW_MANAGEMENT_POLICY_H
#define MIRAL_WINDOW_MANAGEMENT_POLICY_H

#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangles.h>
#include <mir_toolkit/event.h>

#include <memory>

namespace miral
{
class Window;
class WindowSpecification;
struct ApplicationInfo;
struct WindowInfo;

/**
 * Workspace is intentionally opaque in the miral API. Its only purpose is to
 * provide a shared_ptr which is used as an identifier.
 */
class Workspace;

using namespace mir::geometry;

/// The interface through which the window management policy is determined.
class WindowManagementPolicy
{
public:
    /// before any related calls begin
    virtual void advise_begin();

    /// after any related calls end
    virtual void advise_end();

    /** Customize initial window placement
     *
     * @param app_info                the application requesting a new window
     * @param requested_specification the requested specification (updated with default placement)
     * @return                        the customized specification
     */
    virtual auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification = 0;

/** @name handle events originating from the client
 * The policy is expected to update the model as appropriate
 *  @{ */
    /** notification that the first buffer has been posted
     *
     * @param window_info   the window
     */
    virtual void handle_window_ready(WindowInfo& window_info) = 0;

    /** request from client to modify the window specification.
     * \note the request has already been validated against the type definition
     *
     * @param window_info   the window
     * @param modifications the requested changes
     */
    virtual void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) = 0;

    /** request from client to raise the window
     * \note the request has already been validated against the requesting event
     *
     * @param window_info   the window
     */
    virtual void handle_raise_window(WindowInfo& window_info) = 0;
/** @} */

/** @name handle events originating from user
 * The policy is expected to interpret (and optionally consume) the event
 *  @{ */
    /** keyboard event handler
     *
     * @param event the event
     * @return      whether the policy has consumed the event
     */
    virtual bool handle_keyboard_event(MirKeyboardEvent const* event) = 0;

    /** touch event handler
     *
     * @param event the event
     * @return      whether the policy has consumed the event
     */
    virtual bool handle_touch_event(MirTouchEvent const* event) = 0;

    /** pointer event handler
     *
     * @param event the event
     * @return      whether the policy has consumed the event
     */
    virtual bool handle_pointer_event(MirPointerEvent const* event) = 0;
/** @} */

/** @name notification of WM events that the policy may need to track.
 * \note if the policy updates a Window object directly (as opposed to using tools)
 * no notification is generated.
 *  @{ */
    /** Notification that a new application has connected
     *
     * @param application the application
     */
    virtual void advise_new_app(ApplicationInfo& application);

    /** Notification that an application has disconnected
     *
     * @param application the application
     */
    virtual void advise_delete_app(ApplicationInfo const& application);

    /** Notification that a window has been created
     *
     * @param window_info   the window
     */
    virtual void advise_new_window(WindowInfo const& window_info);

    /** Notification that a window has lost focus
     *
     * @param window_info   the window
     */
    virtual void advise_focus_lost(WindowInfo const& window_info);

    /** Notification that a window has gained focus
     *
     * @param window_info   the window
     */
    virtual void advise_focus_gained(WindowInfo const& window_info);

    /** Notification that a window is about to change state
     *
     * @param window_info   the window
     * @param state         the new state
     */
    virtual void advise_state_change(WindowInfo const& window_info, MirWindowState state);

    /** Notification that a window is about to move
     *
     * @param window_info   the window
     * @param top_left      the new position
     */
    virtual void advise_move_to(WindowInfo const& window_info, Point top_left);

    /** Notification that a window is about to resize
     *
     * @param window_info   the window
     * @param new_size      the new size
     */
    virtual void advise_resize(WindowInfo const& window_info, Size const& new_size);

    /** Notification that a window is about to be destroyed
     *
     * @param window_info   the window
     */
    virtual void advise_delete_window(WindowInfo const& window_info);

    /** Notification that windows are being raised to the top.
     *  These windows are ordered with parents before children,
     *  and form a single tree rooted at the first element.
     *
     * @param windows   the windows
     * \note The relative Z-order of these windows will be maintained, they will be raised en bloc.
     */
    virtual void advise_raise(std::vector<Window> const& windows);
/** @} */

/** @name notification of WM events that the policy may need to track.
 *  @{ */

    /** Notification that windows are being added to a workspace.
     *  These windows are ordered with parents before children,
     *  and form a single tree rooted at the first element.
     *
     * @param workspace   the workspace
     * @param windows   the windows
     */
    virtual void advise_adding_to_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows);

    /** Notification that windows are being removed from a workspace.
     *  These windows are ordered with parents before children,
     *  and form a single tree rooted at the first element.
     *
     * @param workspace   the workspace
     * @param windows   the windows
     */
    virtual void advise_removing_from_workspace(
        std::shared_ptr<Workspace> const& workspace,
        std::vector<Window> const& windows);
/** @} */

    /** Confirm (and optionally adjust) the motion of a child window when the parent is moved.
     *
     * @param window_info   the window
     * @param movement      the movement of the parent
     *
     * @return the confirmed placement of the window
     */
    virtual auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle = 0;

    virtual ~WindowManagementPolicy() = default;
    WindowManagementPolicy() = default;
    WindowManagementPolicy(WindowManagementPolicy const&) = delete;
    WindowManagementPolicy& operator=(WindowManagementPolicy const&) = delete;
};

class WindowManagerTools;
}

#endif //MIRAL_WINDOW_MANAGEMENT_POLICY_H
