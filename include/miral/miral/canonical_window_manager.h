/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_CANONICAL_WINDOW_MANAGER_H_
#define MIRAL_CANONICAL_WINDOW_MANAGER_H_

#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>

namespace miral
{
/// Widely accepted defaults for window management
class CanonicalWindowManagerPolicy  : public WindowManagementPolicy
{
public:

    explicit CanonicalWindowManagerPolicy(WindowManagerTools const& tools);

    auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& request_parameters)
        -> WindowSpecification override;

    /// Tries to focus on the newly ready window
    void handle_window_ready(WindowInfo& window_info) override;

    /// Applies the requested modifications
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    /// Tries to focus on the newly ready window
    void handle_raise_window(WindowInfo& window_info) override;

    /// Raises the window (and any children)
    void advise_focus_gained(WindowInfo const& info) override;

    /// Move the child window with the parent
    auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle override;

protected:
    WindowManagerTools tools;
};
}

#endif /* MIRAL_CANONICAL_WINDOW_MANAGER_H_ */
