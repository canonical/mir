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

#ifndef MIRAL_WINDOW_MANAGEMENT_POLICY_ADDENDUM3_H
#define MIRAL_WINDOW_MANAGEMENT_POLICY_ADDENDUM3_H

#include <mir_toolkit/client_types.h>

#include <mir/geometry/rectangles.h>
#include <mir_toolkit/mir_version_number.h>

namespace miral
{
using namespace mir::geometry;
struct WindowInfo;

/**
 *  Handle additional client requests.
 *
 *  \note This interface is intended to be implemented by a WindowManagementPolicy
 *  implementation, we can't add these functions directly to that interface without
 *  breaking ABI (the vtab could be incompatible).
 *  When initializing the window manager this interface will be detected by
 *  dynamic_cast and registered accordingly.
 */
class WindowManagementPolicyAddendum3
{
public:
    /** Confirm (and optionally adjust) the placement of a window on the display.
     * Called when (re)placing fullscreen, maximized, horizontally maximised and
     * vertically maximized windows to allow adjustment for decorations.
     *
     * @param window_info   the window
     * @param new_state     the new state
     * @param new_placement the suggested placement
     *
     * @return the confirmed placement of the window
     */
    virtual auto confirm_placement_on_display(
        WindowInfo const& window_info,
        MirWindowState new_state,
        Rectangle const& new_placement) -> Rectangle = 0;

    virtual ~WindowManagementPolicyAddendum3() = default;
    WindowManagementPolicyAddendum3() = default;
    WindowManagementPolicyAddendum3(WindowManagementPolicyAddendum3 const&) = delete;
    WindowManagementPolicyAddendum3& operator=(WindowManagementPolicyAddendum3 const&) = delete;
};
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(2, 0, 0)
#error "We've presumably broken ABI - please roll this interface into WindowManagementPolicy"
#endif
}

#endif //MIRAL_WINDOW_MANAGEMENT_POLICY_ADDENDUM3_H
