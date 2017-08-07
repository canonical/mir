/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_input_filter.h"

#include "mir/server.h"

#include "mir/compositor/compositor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"

#include <linux/input.h>

#include <iostream>

namespace me = mir::examples;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;

///\example server_example_input_filter.cpp
/// Demonstrate adding a custom input filter to a server
namespace
{
void print_key_event(MirInputEvent const* ev)
{
    auto event_time = mir_input_event_get_event_time(ev);
    auto kev = mir_input_event_get_keyboard_event(ev);
    auto scan_code = mir_keyboard_event_scan_code(kev);
    auto key_code = mir_keyboard_event_key_code(kev);

    std::cout << "Handling key event (time, scancode, keycode): " << event_time << " " <<
              scan_code << " " << key_code << std::endl;
}

void print_touch_event(MirInputEvent const* ev)
{
    auto event_time = mir_input_event_get_event_time(ev);
    auto tev = mir_input_event_get_touch_event(ev);
    auto tc = mir_touch_event_point_count(tev);

    std::cout << "Handling touch event time=" << event_time
              << " touch_count=" << tc << std::endl;
    for (unsigned i = 0; i < tc; i++)
    {
        auto id = mir_touch_event_id(tev, i);
        auto px = mir_touch_event_axis_value(tev, i, mir_touch_axis_x);
        auto py = mir_touch_event_axis_value(tev, i, mir_touch_axis_y);

        std::cout << "  "
            << " id=" << id
            << " pos=(" << px << ", " << py << ")"
            << std::endl;
    }
    std::cout << "----------------" << std::endl << std::endl;
}

void print_pointer_event(MirInputEvent const* ev)
{
    auto event_time = mir_input_event_get_event_time(ev);
    auto pev = mir_input_event_get_pointer_event(ev);
    auto action = mir_pointer_event_action(pev);

    std::cout << "Handling pointer event time=" << event_time
        << " action=";
    switch(action)
    {
    case mir_pointer_action_motion:
        std::cout << "motion";
        break;
    case mir_pointer_action_button_up:
        std::cout << "up";
        break;
    case mir_pointer_action_button_down:
        std::cout << "down";
        break;
    default:
        break;
    }

    std::cout << "  "
              << " pos=(" << mir_pointer_event_axis_value(pev, mir_pointer_axis_x) << ", "
              << mir_pointer_event_axis_value(pev, mir_pointer_axis_y) << ")"
              << " relative=(" << mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_x) << ", "
              << mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_y) << ")" << std::endl;
    std::cout << "----------------" << std::endl << std::endl;
}


struct PrintingEventFilter : public mi::EventFilter
{
    bool handle(MirEvent const& ev) override
    {
        if (mir_event_get_type(&ev) != mir_event_type_input)
            return false;
        auto input_event = mir_event_get_input_event(&ev);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            print_key_event(input_event);
            break;
        case mir_input_event_type_touch:
            print_touch_event(input_event);
            break;
        case mir_input_event_type_pointer:
            print_pointer_event(input_event);
            break;
        default:
            std::cout << "unkown input event type: " << mir_input_event_get_type(input_event) << std::endl;
            break;
        }

        return false;
    }
};

struct ScreenRotationFilter : public mi::EventFilter
{
    bool handle(MirEvent const& event) override
    {
        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        auto const input_event = mir_event_get_input_event(&event);

        if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
            return false;

        return handle_keyboard_event(mir_input_event_get_keyboard_event(input_event));
    }

    bool handle_keyboard_event(MirKeyboardEvent const* event)
    {
        static const int modifier_mask =
            mir_input_event_modifier_alt |
            mir_input_event_modifier_shift |
            mir_input_event_modifier_sym |
            mir_input_event_modifier_ctrl |
            mir_input_event_modifier_meta;

        auto const action = mir_keyboard_event_action(event);
        auto const scan_code = mir_keyboard_event_scan_code(event);
        auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

        if (action == mir_keyboard_action_down &&
            modifiers == (mir_input_event_modifier_alt | mir_input_event_modifier_ctrl))
        {
            switch (scan_code)
            {
            case KEY_UP:
                apply_orientation(mir_orientation_normal);
                break;

            case KEY_DOWN:
                apply_orientation(mir_orientation_inverted);
                break;

            case KEY_LEFT:
                apply_orientation(mir_orientation_left);
                break;

            case KEY_RIGHT:
                apply_orientation(mir_orientation_right);
                break;

            default:
                return false;
            }

            return true;
        }

        return false;
    }

    void apply_orientation(MirOrientation orientation)
    {
        // TODO This is too nuts & bolts for the public API.
        // TODO There should be an interface onto MediatingDisplayChanger that
        // TODO provides equivalent functionality wrapped nicely
        compositor->stop();
        auto conf = display->configuration();

        conf->for_each_output([orientation](mg::UserDisplayConfigurationOutput& output)
            {
                output.orientation = orientation;
            });

        display->configure(*conf);
        compositor->start();
    }

    std::shared_ptr<mg::Display> display;
    std::shared_ptr<mc::Compositor> compositor;
};
}

auto me::make_printing_input_filter_for(mir::Server& server)
-> std::shared_ptr<mi::EventFilter>
{
    static const char* const print_input_events = "print-input-events";
    static const char* const print_input_events_descr = "List input events on std::cout";

    server.add_configuration_option(print_input_events, print_input_events_descr, mir::OptionType::null);

    auto const printing_filter = std::make_shared<PrintingEventFilter>();

    server.add_init_callback([printing_filter, &server]
        {
            const auto options = server.get_options();
            if (options->is_set(print_input_events))
                server.the_composite_event_filter()->prepend(printing_filter);
        });

    return printing_filter;
}

auto me::make_screen_rotation_filter_for(mir::Server& server)
-> std::shared_ptr<input::EventFilter>
{
    static const char* const screen_rotation = "screen-rotation";
    static const char* const screen_rotation_descr = "Rotate screen on Ctrl-Alt-<Arrow>";

    server.add_configuration_option(screen_rotation, screen_rotation_descr, mir::OptionType::null);

    auto const screen_rotation_filter = std::make_shared<ScreenRotationFilter>();

    server.add_init_callback([screen_rotation_filter, &server]
        {
            const auto options = server.get_options();
            if (options->is_set(screen_rotation))
            {
                screen_rotation_filter->display = server.the_display();
                screen_rotation_filter->compositor = server.the_compositor();
                server.the_composite_event_filter()->prepend(screen_rotation_filter);
            }
        });

    return screen_rotation_filter;
}
