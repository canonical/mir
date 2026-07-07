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

#ifndef MIRAL_X11_KIOSK_WINDOW_MANAGER_H
#define MIRAL_X11_KIOSK_WINDOW_MANAGER_H

#include "kiosk_window_manager_policy.h"

class X11KioskWindowManagerPolicy : public KioskWindowManagerPolicyBase
{
public:
    X11KioskWindowManagerPolicy(miral::WindowManagerTools const& tools);

    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;
    void handle_window_ready(miral::WindowInfo& window_info) override;
};

#endif /* MIRAL_X11_KIOSK_WINDOW_MANAGER_H */
