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
#include "demo_compositor.h"

#include "mir/shell/focus_controller.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
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
namespace mi = mir::input;

namespace
{
const int min_swipe_distance = 100;  // How long must a swipe be to act on?
}

me::WindowManager::WindowManager()
    : old_pinch_diam(0.0f), max_fingers(0)
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

void me::WindowManager::set_input_scene(std::shared_ptr<mi::Scene> const& s)
{
    input_scene = s;
}

void me::WindowManager::force_redraw()
{
    // This is clumsy, but the only option our architecture allows us for now
    // Same hack as used in TouchspotController...
    input_scene->emit_scene_changed();
}

namespace
{

mir::geometry::Point average_pointer(MirTouchEvent const* tev)
{
    using namespace mir;
    using namespace geometry;

    int x = 0, y = 0;
    int count = mir_touch_event_point_count(tev);

    for (int i = 0; i < count; i++)
    {
        x += mir_touch_event_axis_value(tev, i, mir_touch_axis_x);
        y += mir_touch_event_axis_value(tev, i, mir_touch_axis_y);
    }

    x /= count;
    y /= count;

    return Point{x, y};
}

float measure_pinch(MirTouchEvent const* tev,
                    mir::geometry::Displacement& dir)
{
    int count = mir_touch_event_point_count(tev);
    int max = 0;

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < i; j++)
        {
            int dx = mir_touch_event_axis_value(tev, i, mir_touch_axis_x) -
                mir_touch_event_axis_value(tev, j, mir_touch_axis_x);
            int dy = mir_touch_event_axis_value(tev, i, mir_touch_axis_y) -
                mir_touch_event_axis_value(tev, j, mir_touch_axis_y);

            int sqr = dx*dx + dy*dy;

            if (sqr > max)
            {
                max = sqr;
                dir = mir::geometry::Displacement{dx, dy};
            }
        }
    }

    return sqrtf(max);  // return pinch diameter
}

} // namespace


void me::WindowManager::toggle(ColourEffect which)
{
    colour_effect = (colour_effect == which) ? none : which;
    me::DemoCompositor::for_each([this](me::DemoCompositor& c)
    {
        c.set_colour_effect(colour_effect);
    });
    force_redraw();
}

void me::WindowManager::save_edges(scene::Surface& surf,
                                   geometry::Point const& p)
{
    int width = surf.size().width.as_int();
    int height = surf.size().height.as_int();

    int left = surf.top_left().x.as_int();
    int right = left + width;
    int top = surf.top_left().y.as_int();
    int bottom = top + height;

    int leftish = left + width/3;
    int rightish = right - width/3;
    int topish = top + height/3;
    int bottomish = bottom - height/3;

    int click_x = p.x.as_int();
    xedge = (click_x <= leftish) ? left_edge :
            (click_x >= rightish) ? right_edge :
            hmiddle;

    int click_y = p.y.as_int();
    yedge = (click_y <= topish) ? top_edge :
            (click_y >= bottomish) ? bottom_edge :
            vmiddle;
}

void me::WindowManager::resize(scene::Surface& surf,
                               geometry::Point const& cursor) const
{
    int width = surf.size().width.as_int();
    int height = surf.size().height.as_int();

    int left = surf.top_left().x.as_int();
    int right = left + width;
    int top = surf.top_left().y.as_int();
    int bottom = top + height;

    geometry::Displacement drag = cursor - old_cursor;
    int dx = drag.dx.as_int();
    int dy = drag.dy.as_int();

    if (xedge == left_edge && dx < width)
        left = old_pos.x.as_int() + dx;
    else if (xedge == right_edge)
        right = old_pos.x.as_int() + old_size.width.as_int() + dx;
    
    if (yedge == top_edge && dy < height)
        top = old_pos.y.as_int() + dy;
    else if (yedge == bottom_edge)
        bottom = old_pos.y.as_int() + old_size.height.as_int() + dy;

    surf.move_to({left, top});
    surf.resize({right-left, bottom-top});
}

bool me::WindowManager::handle_key_event(MirKeyboardEvent const* kev)
{
    // TODO: Fix android configuration and remove static hack ~racarr
    static bool display_off = false;

    if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
        return false;

    auto modifiers = mir_keyboard_event_modifiers(kev);
    auto scan_code = mir_keyboard_event_scan_code(kev);
    
    if (modifiers & mir_input_event_modifier_alt &&
        scan_code == KEY_TAB)  // TODO: Use keycode once we support keymapping on the server side
    {
        focus_controller->focus_next_session();
        if (auto const surface = focus_controller->focused_surface())
            focus_controller->raise({surface});
        return true;
    }
    else if (modifiers & mir_input_event_modifier_alt &&
             scan_code == KEY_GRAVE)
    {
        if (auto const prev = focus_controller->focused_surface())
        {
            auto const app = focus_controller->focused_session();
            auto const next = app->surface_after(prev);
            focus_controller->set_focus_to(app, next);
            focus_controller->raise({next});
        }
        return true;
    }
    else if (modifiers & mir_input_event_modifier_alt &&
             scan_code == KEY_F4)
    {
        auto const surf = focus_controller->focused_surface();
        if (surf)
            surf->request_client_surface_close();
        return true;
    }
    else if ((modifiers & mir_input_event_modifier_alt &&
              scan_code == KEY_P) ||
             (scan_code == KEY_POWER))
    {
        compositor->stop();
        auto conf = display->configuration();
        MirPowerMode new_power_mode = display_off ?
            mir_power_mode_on : mir_power_mode_off;
        conf->for_each_output(
            [&](mg::UserDisplayConfigurationOutput& output) -> void
            {
                output.power_mode = new_power_mode;
            });
        display_off = !display_off;

        display->configure(*conf.get());
        if (!display_off)
            compositor->start();
        return true;
    }
    else if ((modifiers & mir_input_event_modifier_alt) &&
             (modifiers & mir_input_event_modifier_ctrl) &&
             (scan_code == KEY_ESC))
    {
        std::abort();
        return true;
    }
    else if ((modifiers & mir_input_event_modifier_alt) &&
             (modifiers & mir_input_event_modifier_ctrl) &&
             (scan_code == KEY_L) &&
             focus_controller)
    {
        auto const app = focus_controller->focused_session();
        if (app)
        {
            app->set_lifecycle_state(mir_lifecycle_state_will_suspend);
        }
    }
    else if ((modifiers & mir_input_event_modifier_alt) &&
             (modifiers & mir_input_event_modifier_ctrl))
    {
        MirOrientation orientation = mir_orientation_normal;
        bool rotating = true;
        int mode_change = 0;
        bool preferred_mode = false;
        switch (scan_code)
        {
        case KEY_UP:    orientation = mir_orientation_normal; break;
        case KEY_DOWN:  orientation = mir_orientation_inverted; break;
        case KEY_LEFT:  orientation = mir_orientation_left; break;
        case KEY_RIGHT: orientation = mir_orientation_right; break;
        default:        rotating = false; break;
        }
        switch (scan_code)
        {
        case KEY_MINUS: mode_change = -1;      break;
        case KEY_EQUAL: mode_change = +1;      break;
        case KEY_0:     preferred_mode = true; break;
        default:                               break;
        }
        
        if (rotating || mode_change || preferred_mode)
        {
            compositor->stop();
            auto conf = display->configuration();
            conf->for_each_output(
                [&](mg::UserDisplayConfigurationOutput& output) -> void
                {
                    // Only apply changes to the monitor the cursor is on
                    if (!output.extents().contains(old_cursor))
                        return;
                    if (rotating)
                        output.orientation = orientation;
                    if (preferred_mode)
                    {
                        output.current_mode_index =
                            output.preferred_mode_index;
                    }
                    else if (mode_change)
                    {
                        size_t nmodes = output.modes.size();
                        if (nmodes)
                            output.current_mode_index =
                                (output.current_mode_index + nmodes +
                                 mode_change) % nmodes;
                    }
                });
                                  
            display->configure(*conf);
            compositor->start();
            return true;
        }
    }
    else if ((scan_code == KEY_VOLUMEDOWN ||
              scan_code == KEY_VOLUMEUP) &&
             max_fingers == 1)
    {
        int delta = (scan_code == KEY_VOLUMEDOWN) ? -1 : +1;
        static const MirOrientation order[4] =
        {
            mir_orientation_normal,
            mir_orientation_right,
            mir_orientation_inverted,
            mir_orientation_left
        };
        compositor->stop();
        auto conf = display->configuration();
        conf->for_each_output(
            [&](mg::UserDisplayConfigurationOutput& output)
            {
                int i = 0;
                for (; i < 4; ++i)
                {
                    if (output.orientation == order[i])
                        break;
                }
                output.orientation = order[(i+4+delta) % 4];
            });
        display->configure(*conf.get());
        compositor->start();
        return true;
    }
    else if (modifiers & mir_input_event_modifier_meta &&
             scan_code == KEY_N)
    {
        toggle(inverse);
        return true;
    }
    else if (modifiers & mir_input_event_modifier_meta &&
             scan_code == KEY_C)
    {
        toggle(contrast);
        return true;
    }
    return false;
}


bool me::WindowManager::handle_pointer_event(MirPointerEvent const* pev)
{
    bool handled = false;

    geometry::Point cursor{mir_pointer_event_axis_value(pev, mir_pointer_axis_x),
                           mir_pointer_event_axis_value(pev, mir_pointer_axis_y)};
    auto action = mir_pointer_event_action(pev);
    auto modifiers = mir_pointer_event_modifiers(pev);
    auto vscroll = mir_pointer_event_axis_value(pev, mir_pointer_axis_vscroll);
    auto primary_button_pressed = mir_pointer_event_button_state(pev, mir_pointer_button_primary);
    auto tertiary_button_pressed = mir_pointer_event_button_state(pev, mir_pointer_button_tertiary);

    float new_zoom_mag = 0.0f;  // zero means unchanged

   if (modifiers & mir_input_event_modifier_meta &&
       action == mir_pointer_action_motion)
   {
        zoom_exponent += vscroll;

        // Negative exponents do work too, but disable them until
        // there's a clear edge to the desktop.
        if (zoom_exponent < 0)
            zoom_exponent = 0;
    
        new_zoom_mag = powf(1.2f, zoom_exponent);
        handled = true;
    }

    me::DemoCompositor::for_each(
        [new_zoom_mag,&cursor](me::DemoCompositor& c)
        {
            if (new_zoom_mag > 0.0f)
                c.zoom(new_zoom_mag);
            c.on_cursor_movement(cursor);
        });

    if (zoom_exponent || new_zoom_mag)
        force_redraw();

    auto const surf = focus_controller->focused_surface();
    if (surf &&
        (modifiers & mir_input_event_modifier_alt) && (primary_button_pressed || tertiary_button_pressed))
    {
        // Start of a gesture: When the latest finger/button goes down
        if (action == mir_pointer_action_button_down)
        {
            click = cursor;
            save_edges(*surf, click);
            handled = true;
        }
        else if (action == mir_pointer_action_motion)
        {
            geometry::Displacement drag = cursor - old_cursor;

            if (tertiary_button_pressed)
            {  // Resize by mouse middle button
                resize(*surf, cursor);
            }
            else
            {
                surf->move_to(old_pos + drag);
            }

            handled = true;
        }

        old_pos = surf->top_left();
        old_size = surf->size();
    }

    if (surf && 
        (modifiers & mir_input_event_modifier_alt) &&
        action == mir_pointer_action_motion &&
        vscroll)
    {
        float alpha = surf->alpha();
        alpha += 0.1f * vscroll;
        if (alpha < 0.0f)
            alpha = 0.0f;
        else if (alpha > 1.0f)
            alpha = 1.0f;
        surf->set_alpha(alpha);
        handled = true;
    }

    old_cursor = cursor;
    return handled;
}

namespace
{
bool any_touches_went_down(MirTouchEvent const* tev)
{
    auto count = mir_touch_event_point_count(tev);
    for (unsigned i = 0; i < count; i++)
    {
        if (mir_touch_event_action(tev, i) == mir_touch_action_down)
            return true;
    }
    return false;
}
bool last_touch_released(MirTouchEvent const* tev)
{
    auto count = mir_touch_event_point_count(tev);
    if (count > 1)
        return false;
    return mir_touch_event_action(tev, 0) == mir_touch_action_up;
}
}

bool me::WindowManager::handle_touch_event(MirTouchEvent const* tev)
{
    bool handled = false;
    geometry::Point cursor = average_pointer(tev);

    auto const& modifiers = mir_touch_event_modifiers(tev);

    int fingers = mir_touch_event_point_count(tev);

    if (fingers > max_fingers)
        max_fingers = fingers;

    auto const surf = focus_controller->focused_surface();
    if (surf &&
        (modifiers & mir_input_event_modifier_alt ||
         fingers >= 3))
    {
        geometry::Displacement pinch_dir;
        auto pinch_diam =
            measure_pinch(tev, pinch_dir);

        // Start of a gesture: When the latest finger/button goes down
        if (any_touches_went_down(tev))
        {
            click = cursor;
            save_edges(*surf, click);
            handled = true;
        }
        else if(max_fingers <= 3)  // Avoid accidental movement
        {
            geometry::Displacement drag = cursor - old_cursor;

            surf->move_to(old_pos + drag);

            if (fingers == 3)
            {  // Resize by pinch/zoom
                float diam_delta = pinch_diam - old_pinch_diam;
                /*
                 * Resize vector (dx,dy) has length=diam_delta and
                 * direction=pinch_dir, so solve for (dx,dy)...
                 */
                float lenlen = diam_delta * diam_delta;
                int x = pinch_dir.dx.as_int();
                int y = pinch_dir.dy.as_int();
                int xx = x * x;
                int yy = y * y;
                int xxyy = xx + yy;
                int dx = sqrtf(lenlen * xx / xxyy);
                int dy = sqrtf(lenlen * yy / xxyy);
                if (diam_delta < 0.0f)
                {
                    dx = -dx;
                    dy = -dy;
                }

                int width = old_size.width.as_int() + dx;
                int height = old_size.height.as_int() + dy;
                surf->resize({width, height});
            }

            handled = true;
        }

        old_pos = surf->top_left();
        old_size = surf->size();
        old_pinch_diam = pinch_diam;
    }

    auto gesture_ended = last_touch_released(tev);

    if (max_fingers == 4 && gesture_ended)
    { // Four fingers released
        geometry::Displacement dir = cursor - click;
        if (abs(dir.dx.as_int()) >= min_swipe_distance)
        {
            focus_controller->focus_next_session();
            if (auto const surface = focus_controller->focused_surface())
                focus_controller->raise({surface});
            handled = true;
        }
    }

    if (fingers == 1 && gesture_ended)
        max_fingers = 0;

    old_cursor = cursor;
    return handled;
}

bool me::WindowManager::handle(MirEvent const& event)
{
    assert(focus_controller);
    assert(display);
    assert(compositor);

    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;
    auto iev = mir_event_get_input_event(&event);
    auto input_type = mir_input_event_get_type(iev);
    
    if (input_type == mir_input_event_type_key)
    {
        return handle_key_event(mir_input_event_get_keyboard_event(iev));
    }
    else if (input_type == mir_input_event_type_pointer &&
             focus_controller)
    {
        return handle_pointer_event(mir_input_event_get_pointer_event(iev));
    }
    else if (input_type == mir_input_event_type_touch &&
             focus_controller)
    {
        return handle_touch_event(mir_input_event_get_touch_event(iev));
    }


    return false;
}
