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

#ifndef MIRAL_KIOSK_WINDOW_MANAGER_H
#define MIRAL_KIOSK_WINDOW_MANAGER_H

#include <miral/canonical_window_manager.h>

namespace miral
{
/// Base class for kiosk-style window managers with common input handling
/// Subclasses pass the target window state (fullscreen or maximized) to the constructor.
class KioskWindowManagerPolicy : public CanonicalWindowManagerPolicy
{
public:
    /// \param tools window manager tools
    /// The normal target window state for kiosk is mir_window_state_fullscreen
    KioskWindowManagerPolicy(WindowManagerTools const& tools);

    /// \param tools window manager tools
    /// \param target_state the target window state for kiosk mode (e.g., mir_window_state_fullscreen or mir_window_state_maximized)
    KioskWindowManagerPolicy(WindowManagerTools const& tools, MirWindowState target_state);

    auto place_new_window(ApplicationInfo const& app_info, WindowSpecification const& request)
        -> WindowSpecification override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_touch_event(MirTouchEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;
    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

protected:
    MirWindowState const target_state;
};
}

#endif /* MIRAL_KIOSK_WINDOW_MANAGER_H */
