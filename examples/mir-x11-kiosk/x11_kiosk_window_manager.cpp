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

#include "x11_kiosk_window_manager.h"

#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

using namespace miral;

X11KioskWindowManagerPolicy::X11KioskWindowManagerPolicy(WindowManagerTools const& tools) :
    KioskWindowManagerPolicyBase{tools, mir_window_state_fullscreen}
{
}

void X11KioskWindowManagerPolicy::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications)
{
    WindowSpecification specification = modifications;

    if ((window_info.type() == mir_window_type_normal || window_info.type() == mir_window_type_freestyle) &&
        !window_info.parent())
    {
        if (window_info.state() == mir_window_state_fullscreen && modifications.state().has_value())
        {
            // If the window is already fullscreen, then first restore it
            // as otherwise the client may not repaint
            WindowSpecification mods;
            mods.state() = mir_window_state_restored;

            tools.place_and_size_for_state(mods, window_info);
            tools.modify_window(window_info, mods);
        }

        if (window_info.is_visible() || !modifications.state().has_value() || modifications.state().value() != mir_window_state_restored)
        {
            specification.state() = mir_window_state_fullscreen;
            specification.size() = std::nullopt; // Ignore requested size (if any) when we fullscreen
            specification.top_left() = std::nullopt; // Ignore requested position (if any) when we fullscreen
            tools.place_and_size_for_state(specification, window_info);

            if (!modifications.state().has_value() || modifications.state().value() != mir_window_state_restored)
                specification.state() = modifications.state();
        }
        else
        {
            // We have one X11 application (vmware-view) that doesn't submit any buffers unless it
            // can first "restore" its window
            tools.place_and_size_for_state(specification, window_info);
        }
    }

    KioskWindowManagerPolicyBase::handle_modify_window(window_info, specification);
}

void X11KioskWindowManagerPolicy::handle_window_ready(WindowInfo& window_info)
{
    if ((window_info.type() == mir_window_type_normal || window_info.type() == mir_window_type_freestyle) &&
        !window_info.parent())
    {
        // We have one X11 application (vmware-view) that doesn't submit any buffers unless it
        // can first "restore" its window. We've got a buffer now, so we can fullscreen it
        if (window_info.state() == mir_window_state_restored)
        {
            WindowSpecification specification;
            specification.state() = mir_window_state_fullscreen;
            tools.place_and_size_for_state(specification, WindowInfo{});
            tools.modify_window(window_info, specification);
        }
    }
    KioskWindowManagerPolicyBase::handle_window_ready(window_info);
}
