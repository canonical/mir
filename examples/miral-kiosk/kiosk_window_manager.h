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

#include "sw_splash.h"
#include "kiosk_window_manager_policy.h"

#include <mir_toolkit/events/enums.h>

using namespace mir::geometry;

class KioskWindowManagerPolicy : public KioskWindowManagerPolicyBase
{
public:
    KioskWindowManagerPolicy(miral::WindowManagerTools const& tools, std::shared_ptr<SplashSession> const&);

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    void advise_focus_gained(miral::WindowInfo const& info) override;


private:
    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    std::shared_ptr<SplashSession> const splash;
};

#endif /* MIRAL_KIOSK_WINDOW_MANAGER_H */
