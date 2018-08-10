/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_TEST_FRAMEWORK_CANONICAL_WINDOW_MANAGER_POLICY_H
#define MIR_TEST_FRAMEWORK_CANONICAL_WINDOW_MANAGER_POLICY_H

#include <miral/canonical_window_manager.h>

namespace mir_test_framework
{
struct CanonicalWindowManagerPolicy : miral::CanonicalWindowManagerPolicy
{
    using miral::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy;

    bool handle_keyboard_event(MirKeyboardEvent const*) override { return false; }

    bool handle_pointer_event(MirPointerEvent const*) override { return false; }

    bool handle_touch_event(MirTouchEvent const*) override { return false; }

    mir::geometry::Rectangle confirm_placement_on_display(
        miral::WindowInfo const&,
        MirWindowState,
        mir::geometry::Rectangle const& new_placement) override
    {
        return new_placement;
    }
};
}

#endif //MIR_TEST_FRAMEWORK_CANONICAL_WINDOW_MANAGER_POLICY_H
