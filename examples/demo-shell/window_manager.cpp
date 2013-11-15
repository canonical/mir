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
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "window_manager.h"

#include "mir/shell/focus_controller.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/compositor/compositor.h"

#include <linux/input.h>

#include <cassert>
#include <cstdlib>
#include <cmath>

namespace me = mir::examples;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

namespace
{
const int min_swipe_distance = 100;  // How long must a swipe be to act on?
}

me::WindowManager::WindowManager()
    : old_pinch(0.0f), max_fingers(0)
{
}

void me::WindowManager::set_focus_controller(std::shared_ptr<msh::FocusController> const& controller)
{
    focus_controller = controller;
}

void me::WindowManager::set_display(std::shared_ptr<mg::Display> const& dpy)
{
    display = dpy;
}

void me::WindowManager::set_compositor(std::shared_ptr<mc::Compositor> const& cptor)
{
    compositor = cptor;
}

mir::geometry::Point average_pointer(MirMotionEvent const& motion)
{
    using namespace mir;
    using namespace geometry;

    int x = 0, y = 0, count = static_cast<int>(motion.pointer_count);

    for (int i = 0; i < count; i++)
    {
        x += motion.pointer_coordinates[i].x;
        y += motion.pointer_coordinates[i].y;
    }

    x /= count;
    y /= count;

    return Point{x, y};
}

float pinch_diameter(MirMotionEvent const& motion)
{
    int count = static_cast<int>(motion.pointer_count);
    int max = 0;

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < i; j++)
        {
            int dx = motion.pointer_coordinates[i].x -
                     motion.pointer_coordinates[j].x;
            int dy = motion.pointer_coordinates[i].y -
                     motion.pointer_coordinates[j].y;

            int sqr = dx*dx + dy*dy;

            if (sqr > max)
                max = sqr;
        }
    }

    return sqrtf(max);
}

bool me::WindowManager::handle(MirEvent const& event)
{
    // TODO: Fix android configuration and remove static hack ~racarr
    static bool display_off = false;
    assert(focus_controller);
    assert(display);
    assert(compositor);

    if (event.key.type == mir_event_type_key &&
        event.key.action == mir_key_action_down)
    {
        if (event.key.modifiers & mir_key_modifier_alt &&
            event.key.scan_code == KEY_TAB)  // TODO: Use keycode once we support keymapping on the server side
        {
            focus_controller->focus_next();
            return true;
        }
        if (event.key.modifiers & mir_key_modifier_alt && event.key.scan_code == KEY_P)
        {
            compositor->stop();
            auto conf = display->configuration();
            conf->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
            {
                MirPowerMode power_mode;
                if (!output.used) return;

                if (display_off == true)
                    power_mode = mir_power_mode_on;
                else
                    power_mode = mir_power_mode_off;

                conf->configure_output(output.id, output.used,
                                       output.top_left, 
                                       output.current_mode_index,
                                       power_mode);
            });
            display_off = !display_off;

            display->configure(*conf.get());
            compositor->start();
            return true;
        }
    }
    else if (event.type == mir_event_type_motion &&
             focus_controller)
    {
        geometry::Point cursor = average_pointer(event.motion);

        // FIXME: https://bugs.launchpad.net/mir/+bug/1197108
        MirMotionAction action = static_cast<MirMotionAction>(event.motion.action & ~0xff00);

        std::shared_ptr<msh::Session> app =
            focus_controller->focussed_application().lock();

        int fingers = static_cast<int>(event.motion.pointer_count);

        if (action == mir_motion_action_down || fingers > max_fingers)
            max_fingers = fingers;

        if (app)
        {
            // FIXME: We need to be able to select individual surfaces in
            //        future and not just the "default" one.
            std::shared_ptr<msh::Surface> surf = app->default_surface();

            if (surf &&
                (event.motion.modifiers & mir_key_modifier_alt ||
                 fingers >= 3))
            {
                auto pinch = pinch_diameter(event.motion);

                // Start of a gesture: When the latest finger/button goes down
                if (action == mir_motion_action_down ||
                    action == mir_motion_action_pointer_down)
                {
                    relative_click = cursor - surf->top_left();
                    old_size = surf->size();
                    old_pinch = pinch;
                    click = cursor;
                }
                else if (event.motion.action == mir_motion_action_move &&
                         max_fingers <= 3)  // Avoid accidental movement
                {
                    if (event.motion.button_state == mir_motion_button_tertiary)
                    {  // Resize by mouse middle button
                        geometry::Displacement dir = cursor - click;
                        int width = old_size.width.as_int() + dir.dx.as_int();
                        int height = old_size.height.as_int() + dir.dy.as_int();
                        if (width <= 0) width = 1;
                        if (height <= 0) height = 1; 
                        surf->resize({width, height});
                    }
                    else
                    { // Move surface (by mouse or 3 fingers)
                        geometry::Point abs = cursor - relative_click;
                        surf->move_to(abs);
                    }

                    if (fingers == 3)
                    {  // Resize by pinch/zoom
                        auto pinch_delta = pinch - old_pinch;
                        int width = old_size.width.as_int() + pinch_delta;
                        int height = old_size.height.as_int() + pinch_delta;
                        if (width <= 0) width = 1;
                        if (height <= 0) height = 1; 
                        surf->resize({width, height});
                    }
                    return true;
                }
            }
        }

        if (max_fingers == 4 && action == mir_motion_action_up)
        { // Four fingers released
            geometry::Displacement dir = cursor - click;
            if (abs(dir.dx.as_int()) >= min_swipe_distance)
            {
                focus_controller->focus_next();
                return true;
            }
        }
    }
    return false;
}
