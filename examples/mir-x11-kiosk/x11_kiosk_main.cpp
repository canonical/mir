/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
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

#include <miral/command_line_option.h>
#include <miral/display_configuration.h>
#include <miral/keymap.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/x11_support.h>

int main(int argc, char const* argv[])
{
    using namespace miral;
    MirRunner runner{argc, argv};

    DisplayConfiguration display_config{runner};
 
    return runner.run_with(
        {
            display_config,
            display_config.layout_option(),
            set_window_management_policy<X11KioskWindowManagerPolicy>(),
            Keymap{},
            X11Support{}
        });
}
