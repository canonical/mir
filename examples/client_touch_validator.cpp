/*
 * Client which validates incoming touch events.
 *
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#include "eglapp.h"

#include <mir_toolkit/mir_client_library.h>

#include <GLES2/gl2.h>

#include <set>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace
{
// Assumes all touch input comes from a single device.
// Validates:
//     1. All touches which were down stay down unless they were coming up
//     2. All touches which were released do not appear
//     3. Only one touch comes up or down at a time.

bool validate_events(MirTouchEvent const* last_event, MirTouchEvent const* event)
{
    std::set<MirTouchId> must_be_present;
    std::set<MirTouchId> may_not_be_present;

    // Here, from the last event, we build a list of points which must be
    // and which must not be present in the next event.
    for (size_t i = 0; i < mir_touch_event_point_count(last_event); i++)
    {
        auto id = mir_touch_event_id(last_event, i);
        auto action = mir_touch_event_action(last_event, i);
        if (action == mir_touch_action_change)
            must_be_present.insert(id);
        else if (action == mir_touch_action_down)
            must_be_present.insert(id);
        else
            may_not_be_present.insert(id);
    }
    if (may_not_be_present.size() > 1)
    {
        printf("More than one touch came up\n");
        return false;
    }

    bool found_a_up_down = false;
    for (size_t i = 0; i < mir_touch_event_point_count(event); i++)
    {
        auto id = mir_touch_event_id(event, i);
        auto it = may_not_be_present.find(id);
        // Here we validate condition 2, where released touches may not reappear
        if (it != may_not_be_present.end())
        {
            printf("We repeated a touch which was already lifted (%d)\n", static_cast<int>(id));
            return false;
        }
        it = must_be_present.find(id);
        if (it != must_be_present.end())
        {
            must_be_present.erase(it);
        }
        // Here we validate condition 4
        else if (mir_touch_event_action(event, i) == mir_touch_action_down) 
        {
            if (found_a_up_down)
                printf("Found too many downs in one event\n");
            found_a_up_down = true;
        }
        if (mir_touch_event_action(event, i) == mir_touch_action_up)
        {
            if (found_a_up_down)
                printf("Found too many ups in one event\n");
            found_a_up_down = true;
        }
    }

    // Here we validate condition 1
    if (must_be_present.size())
    {
        printf("We received a touch which did not contain all required IDs\n");
        return false;
    }
    
    return true;
}
    
class TouchState
{
public:
    TouchState() : last_event(nullptr) {}
    ~TouchState() { if (last_event) mir_event_unref(last_event); }
    
    void record_event(MirTouchEvent const* event)
    {
        if (!last_event)
        {
            last_event = mir_event_ref(reinterpret_cast<MirEvent const*>(event));
            return;
        }
        if (!validate_events(reinterpret_cast<MirTouchEvent const*>(last_event), event))
            abort();
        
        mir_event_unref(last_event);
        last_event = mir_event_ref(reinterpret_cast<MirEvent const*>(event));
    }
private:
    MirEvent const* last_event;
};

void on_input_event(TouchState *state, MirInputEvent const *event)
{
    if (mir_input_event_get_type(event) != mir_input_event_type_touch)
        return;
    auto tev = mir_input_event_get_touch_event(event);
    state->record_event(tev);
}
    
void on_event(MirWindow *window, const MirEvent *event, void *context)
{
    TouchState *state = (TouchState*)context;

    switch (mir_event_get_type(event))
    {
    case mir_event_type_input:
        on_input_event(state, mir_event_get_input_event(event));
        break;
    case mir_event_type_resize:
        egl_app_handle_resize_event(window, mir_event_get_resize_event(event));
        break;
    case mir_event_type_close_window:
        abort();
        break;
    default:
        break;
    }
}
}


typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

int main(int argc, char *argv[])
{
    unsigned int width = 0, height = 0;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    TouchState state;

    MirWindow* window = mir_eglapp_native_window();
    mir_window_set_event_handler(window, on_event, &state);

    float const opacity = mir_eglapp_background_opacity;
    Color red = {opacity, 0.0f, 0.0f, opacity};
    Color green = {0.0f, opacity, 0.0f, opacity};
    Color blue = {0.0f, 0.0f, opacity, opacity};

    /* This is probably the simplest GL you can do */
    while (mir_eglapp_running())
    {
        glClearColor(red.r, red.g, red.b, red.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(green.r, green.g, green.b, green.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(blue.r, blue.g, blue.b, blue.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);
    }

    mir_eglapp_cleanup();

    return 0;
}
