/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wl_pointer.h"

#include "wayland_utils.h"
#include "wl_surface.h"

#include "mir/executor.h"
#include "mir/client/event.h"

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;

mf::WlPointer::WlPointer(
    wl_client* client,
    wl_resource* parent,
    uint32_t id,
    std::function<void(WlPointer*)> const& on_destroy,
    std::shared_ptr<mir::Executor> const& executor)
    : Pointer(client, parent, id),
        display{wl_client_get_display(client)},
        executor{executor},
        on_destroy{on_destroy},
        destroyed{std::make_shared<bool>(false)}
{
}

mf::WlPointer::~WlPointer()
{
    *destroyed = true;
    on_destroy(this);
}

void mf::WlPointer::handle_event(MirInputEvent const* event, wl_resource* target)
{
    executor->spawn(run_unless(
        destroyed,
        [
            ev = mir::client::Event{mir_input_event_get_event(event)},
            target,
            target_window_destroyed = WlSurface::from(target)->destroyed_flag(),
            this
        ]()
        {
            if (*target_window_destroyed)
                return;

            auto const serial = wl_display_next_serial(display);
            auto const event = mir_event_get_input_event(ev);
            auto const pointer_event = mir_input_event_get_pointer_event(event);

            switch(mir_pointer_event_action(pointer_event))
            {
                case mir_pointer_action_button_down:
                case mir_pointer_action_button_up:
                {
                    auto const current_set  = mir_pointer_event_buttons(pointer_event);
                    auto const current_time = mir_input_event_get_event_time(event) / 1000;

                    for (auto const& mapping :
                        {
                            std::make_pair(mir_pointer_button_primary, BTN_LEFT),
                            std::make_pair(mir_pointer_button_secondary, BTN_RIGHT),
                            std::make_pair(mir_pointer_button_tertiary, BTN_MIDDLE),
                            std::make_pair(mir_pointer_button_back, BTN_BACK),
                            std::make_pair(mir_pointer_button_forward, BTN_FORWARD),
                            std::make_pair(mir_pointer_button_side, BTN_SIDE),
                            std::make_pair(mir_pointer_button_task, BTN_TASK),
                            std::make_pair(mir_pointer_button_extra, BTN_EXTRA)
                        })
                    {
                        if (mapping.first & (current_set ^ last_set))
                        {
                            auto const action = (mapping.first & current_set) ?
                                                WL_POINTER_BUTTON_STATE_PRESSED :
                                                WL_POINTER_BUTTON_STATE_RELEASED;

                            wl_pointer_send_button(resource, serial, current_time, mapping.second, action);
                        }
                    }

                    last_set = current_set;
                    break;
                }
                case mir_pointer_action_enter:
                {
                    wl_pointer_send_enter(
                        resource,
                        serial,
                        target,
                        wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)),
                        wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)));
                    break;
                }
                case mir_pointer_action_leave:
                {
                    wl_pointer_send_leave(
                        resource,
                        serial,
                        target);
                    break;
                }
                case mir_pointer_action_motion:
                {
                    auto x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x);
                    auto y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y);
                    auto vscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll);
                    auto hscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll);

                    if ((x != last_x) || (y != last_y))
                    {
                        wl_pointer_send_motion(
                            resource,
                            mir_input_event_get_event_time(event) / 1000,
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));

                        last_x = x;
                        last_y = y;
                    }
                    if (vscroll != last_vscroll)
                    {
                        wl_pointer_send_axis(
                            resource,
                            mir_input_event_get_event_time(event) / 1000,
                            WL_POINTER_AXIS_VERTICAL_SCROLL,
                            wl_fixed_from_double(vscroll));
                        last_vscroll = vscroll;
                    }
                    if (hscroll != last_hscroll)
                    {
                        wl_pointer_send_axis(
                            resource,
                            mir_input_event_get_event_time(event) / 1000,
                            WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                            wl_fixed_from_double(hscroll));
                        last_hscroll = hscroll;
                    }
                    break;
                }
                case mir_pointer_actions:
                    break;
            }
        }));
}

void mf::WlPointer::set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y)
{
    (void)serial;
    (void)surface;
    (void)hotspot_x;
    (void)hotspot_y;
}

void mf::WlPointer::release()
{
    wl_resource_destroy(resource);
}
