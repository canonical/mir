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
#include "mir/frontend/session.h"
#include "mir/frontend/surface.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/geometry/displacement.h"

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
using namespace mir::geometry;

struct mf::WlPointer::Cursor
{
    virtual void apply_to(wl_resource* target) = 0;
    virtual ~Cursor() = default;
    Cursor() = default;

    Cursor(Cursor const&) = delete;
    Cursor& operator=(Cursor const&) = delete;
};

namespace
{
struct NullCursor : mf::WlPointer::Cursor
{
    void apply_to(wl_resource*) override {}
};
}

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
        destroyed{std::make_shared<bool>(false)},
        cursor{std::make_unique<NullCursor>()}
{
}

mf::WlPointer::~WlPointer()
{
    *destroyed = true;
    on_destroy(this);
}

void mf::WlPointer::handle_button(uint32_t time, uint32_t button, bool is_pressed)
{
    auto const serial = wl_display_next_serial(display);
    auto const action = is_pressed ?
                        WL_POINTER_BUTTON_STATE_PRESSED :
                        WL_POINTER_BUTTON_STATE_RELEASED;
    wl_pointer_send_button(resource, serial, time, button, action);
    if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
        wl_pointer_send_frame(resource);
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

            cursor->apply_to(target);

            auto const serial = wl_display_next_serial(display);
            auto const event = mir_event_get_input_event(ev);
            auto const pointer_event = mir_input_event_get_pointer_event(event);
            auto const buffer_offset = WlSurface::from(target)->buffer_offset();

            switch(mir_pointer_event_action(pointer_event))
            {
                case mir_pointer_action_button_down:
                case mir_pointer_action_button_up:
                {
                    auto const current_set  = mir_pointer_event_buttons(pointer_event);
                    auto const current_time = mir_input_event_get_event_time_ms(event);

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
                            if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                                wl_pointer_send_frame(resource);
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
                        wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)-buffer_offset.dx.as_int()),
                        wl_fixed_from_double(mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)-buffer_offset.dy.as_int()));
                    if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                        wl_pointer_send_frame(resource);
                    break;
                }
                case mir_pointer_action_leave:
                {
                    wl_pointer_send_leave(
                        resource,
                        serial,
                        target);
                    if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                        wl_pointer_send_frame(resource);
                    break;
                }
                case mir_pointer_action_motion:
                {
                    // TODO: properly group vscroll and hscroll events in the same frame (as described by the frame
                    //  event description in wayland.xml) and send axis_source, axis_stop and axis_discrete events where
                    //  appropriate (may require significant reworking of the input system)

                    auto x = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)-buffer_offset.dx.as_int();
                    auto y = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)-buffer_offset.dy.as_int();

                    // libinput < 0.8 sent wheel click events with value 10. Since 0.8 the value is the angle of the click in
                    // degrees. To keep backwards-compat with existing clients, we just send multiples of the click count.
                    // Ref: https://github.com/wayland-project/weston/blob/master/libweston/libinput-device.c#L184
                    auto vscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll) * 10;
                    auto hscroll = mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll) * 10;

                    if ((x != last_x) || (y != last_y))
                    {
                        wl_pointer_send_motion(
                            resource,
                            mir_input_event_get_event_time_ms(event),
                            wl_fixed_from_double(x),
                            wl_fixed_from_double(y));

                        last_x = x;
                        last_y = y;

                        if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                            wl_pointer_send_frame(resource);
                    }
                    if (vscroll != 0)
                    {
                        wl_pointer_send_axis(
                            resource,
                            mir_input_event_get_event_time_ms(event),
                            WL_POINTER_AXIS_VERTICAL_SCROLL,
                            wl_fixed_from_double(vscroll));

                        if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                            wl_pointer_send_frame(resource);
                    }
                    if (hscroll != 0)
                    {
                        wl_pointer_send_axis(
                            resource,
                            mir_input_event_get_event_time_ms(event),
                            WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                            wl_fixed_from_double(hscroll));

                        if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION)
                            wl_pointer_send_frame(resource);
                    }
                    break;
                }
                case mir_pointer_actions:
                    break;
            }
        }));
}

namespace
{
struct WlStreamCursor : mf::WlPointer::Cursor
{
    WlStreamCursor(std::shared_ptr<mf::Session> const session, std::shared_ptr<mf::BufferStream> const& stream, Displacement hotspot);
    void apply_to(wl_resource* target) override;

    std::shared_ptr<mf::Session>        const session;
    std::shared_ptr<mf::BufferStream>   const stream;
    Displacement        const hotspot;
};

struct WlHiddenCursor : mf::WlPointer::Cursor
{
    WlHiddenCursor(std::shared_ptr<mf::Session> const session);
    void apply_to(wl_resource* target) override;

    std::shared_ptr<mf::Session>        const session;
};
}


void mf::WlPointer::set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y)
{
    if (surface)
    {
        auto const cursor_stream = WlSurface::from(*surface)->stream;
        Displacement const cursor_hotspot{hotspot_x, hotspot_y};

        cursor = std::make_unique<WlStreamCursor>(get_session(client), cursor_stream, cursor_hotspot);
    }
    else
    {
        cursor = std::make_unique<WlHiddenCursor>(get_session(client));
    }

    (void)serial;
}

void mf::WlPointer::release()
{
    wl_resource_destroy(resource);
}

WlStreamCursor::WlStreamCursor(
    std::shared_ptr<mf::Session> const session,
    std::shared_ptr<mf::BufferStream> const& stream,
    Displacement hotspot) :
    session{session},
    stream{stream},
    hotspot{hotspot}
{
}

void WlStreamCursor::apply_to(wl_resource* target)
{
    auto const mir_window = session->get_surface(mf::WlSurface::from(target)->surface_id);
    mir_window->set_cursor_stream(stream, hotspot);
}

WlHiddenCursor::WlHiddenCursor(
    std::shared_ptr<mf::Session> const session) :
    session{session}
{
}

void WlHiddenCursor::apply_to(wl_resource* target)
{
    auto const mir_window = session->get_surface(mf::WlSurface::from(target)->surface_id);
    mir_window->set_cursor_image({});
}