/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "application_switcher.h"

#include "mir/shell/focus_controller.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"

#include <linux/input.h>

#include <assert.h>
#include <cstdio>

namespace me = mir::examples;
namespace msh = mir::shell;

me::ApplicationSwitcher::ApplicationSwitcher()
{
}

void me::ApplicationSwitcher::set_focus_controller(std::shared_ptr<msh::FocusController> const& controller)
{
    focus_controller = controller;
}

void me::ApplicationSwitcher::set_session_manager(
    std::shared_ptr<msh::SessionManager> const& sm)
{
    session_manager = sm;
}

bool me::ApplicationSwitcher::handles(MirEvent const& event)
{
    assert(focus_controller);

    if (event.key.type == mir_event_type_key &&
        event.key.action == mir_key_action_down &&
        event.key.modifiers & mir_key_modifier_alt &&
        event.key.scan_code == KEY_TAB)  // TODO: Use keycode once we support keymapping on the server side
    {
        focus_controller->focus_next();
        return true;
    }
    else if (event.type == mir_event_type_motion &&
             session_manager)
    {
        printf("vv: %lu: pointers=%d\n",
            (unsigned long)event.motion.event_time,
            (int)event.motion.pointer_count);

        std::shared_ptr<msh::Session> app =
            session_manager->focussed_application().lock();
        printf("vv: app is %p\n", (void*)app.get());
        if (app)
        {
            // Default surface == front/focussed?!
            std::shared_ptr<msh::Surface> surf = app->default_surface();
            printf("vv:    --> surf %p\n", (void*)surf.get());
        }
        fflush(stdout);
    }
    return false;
}
