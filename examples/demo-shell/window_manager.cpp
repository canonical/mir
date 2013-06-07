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

#include "window_manager.h"

#include "mir/shell/focus_controller.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"

#include <linux/input.h>

#include <assert.h>

namespace me = mir::examples;
namespace msh = mir::shell;

me::WindowManager::WindowManager()
{
}

void me::WindowManager::set_focus_controller(std::shared_ptr<msh::FocusController> const& controller)
{
    focus_controller = controller;
}

void me::WindowManager::set_session_manager(
    std::shared_ptr<msh::SessionManager> const& sm)
{
    session_manager = sm;
}

bool me::WindowManager::handle(MirEvent const& event)
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
        std::shared_ptr<msh::Session> app =
            session_manager->focussed_application().lock();

        if (app)
        {
            // FIXME: We need to be able to select individual surfaces in
            //        future and not just the "default" one.
            std::shared_ptr<msh::Surface> surf = app->default_surface();

            if (surf &&
                event.motion.modifiers & mir_key_modifier_alt)
            {
                geometry::Point cursor{
                    geometry::X{event.motion.pointer_coordinates[0].x},
                    geometry::Y{event.motion.pointer_coordinates[0].y}};

                if (event.motion.button_state == 0)
                {
                    relative_click = cursor - surf->top_left();
                }
                else if (event.motion.button_state & mir_motion_button_primary)
                {
                    geometry::Point abs = cursor - relative_click;
                    surf->move_to(abs);
                    return true;
                }
            }
        }
    }
    return false;
}
