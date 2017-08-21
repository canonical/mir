/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIRAL_WORKSPACE_POLICY_H
#define MIRAL_WORKSPACE_POLICY_H

#include "miral/version.h"

#include <memory>
#include <vector>

namespace miral
{
class Window;

/**
 * Workspace is intentionally opaque in the miral API. Its only purpose is to
 * provide a shared_ptr which is used as an identifier.
 */
class Workspace;

/**
 *  Advise changes to workspaces.
 *
 *  \note This interface is intended to be implemented by a WindowManagementPolicy
 *  implementation, we can't add these functions directly to that interface without
 *  breaking ABI (the vtab could be incompatible).
 *  When initializing the window manager this interface will be detected by
 *  dynamic_cast and registered accordingly.
 */
class WorkspacePolicy
{
public:
/** @name notification of WM events that the policy may need to track.
 * These calls happen "under lock" and are wrapped by the usual
 * WindowManagementPolicy::advise_begin(), advise_end() calls.
 * They should not call WindowManagerTools::invoke_under_lock()
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

    virtual ~WorkspacePolicy() = default;
    WorkspacePolicy() = default;
    WorkspacePolicy(WorkspacePolicy const&) = delete;
    WorkspacePolicy& operator=(WorkspacePolicy const&) = delete;
};
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(2, 0, 0)
#error "We've presumably broken ABI - please roll this interface into WindowManagementPolicy"
#endif
}
#endif //MIRAL_WORKSPACE_POLICY_H
