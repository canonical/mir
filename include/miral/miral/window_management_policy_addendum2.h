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

#ifndef MIRAL_WINDOW_MANAGEMENT_ADDENDUM2_H
#define MIRAL_WINDOW_MANAGEMENT_ADDENDUM2_H

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/mir_version_number.h>

namespace miral
{
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
class WindowManagementPolicyAddendum2
{
public:
/** @name handle requests originating from the client
 * The policy is expected to update the model as appropriate
 *  @{ */
    /** request from client to initiate drag and drop
     * \note the request has already been validated against the requesting event
     *
     * @param window_info   the window
     */
    virtual void handle_request_drag_and_drop(WindowInfo& window_info) = 0;

    /** request from client to initiate move
     * \note the request has already been validated against the requesting event
     *
     * @param window_info   the window
     * @param input_event   the requesting event
     */
    virtual void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) = 0;
/** @} */

    virtual ~WindowManagementPolicyAddendum2() = default;
    WindowManagementPolicyAddendum2() = default;
    WindowManagementPolicyAddendum2(WindowManagementPolicyAddendum2 const&) = delete;
    WindowManagementPolicyAddendum2& operator=(WindowManagementPolicyAddendum2 const&) = delete;
};
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(2, 0, 0)
#error "We've presumably broken ABI - please roll this interface into WindowManagementPolicy"
#endif
}

#endif //MIRAL_WINDOW_MANAGEMENT_ADDENDUM2_H
