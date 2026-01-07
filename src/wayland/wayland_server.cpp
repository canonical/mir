/*
 * Copyright © Canonical Ltd.
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
 */
#include "mir/wayland/wayland_server.h"
#include "wayland_rs/src/lib.rs.h"

// In the Rust version of wayland-server, the event
// loop is handled in the display.
struct wl_event_loop
{
    rust::Box<EventLoop> event_loop;
};

struct wl_display
{
    rust::Box<DisplayWrapper> wrapper;
    wl_event_loop event_loop;
};

struct wl_event_source
{
    wl_event_loop* event_loop;
    int fd;
};

wl_display* wl_display_create()
{
    auto const result = new wl_display
    {
        create_display_wrapper(),
        {create_event_loop()}
    };

    result->event_loop.event_loop->add_display_fd(*result->wrapper.into_raw());
    return result;
}

void wl_display_destroy(wl_display* display)
{
    delete display; // implicitly drops the box
}

wl_event_loop* wl_display_get_event_loop(wl_display* display)
{
    return &display->event_loop;
}

void wl_display_add_socket(wl_display* display, const char* name)
{

}

void wl_display_terminate(wl_display* display)
{

}

wl_event_loop* wl_event_loop_create()
{
    return new wl_event_loop{create_event_loop()};
}

void wl_event_loop_destroy(wl_event_loop* event_loop)
{
    delete event_loop;
}

wl_event_source* wl_event_loop_add_fd(
    wl_event_loop *loop,
    int fd,
    uint32_t mask,
    wl_event_loop_fd_func_t func,
    void *data)
{
    if (loop->event_loop->add_fd(
        fd,
        mask,
        reinterpret_cast<size_t>(func),
        reinterpret_cast<size_t>(data)
    ))
    {
        return new wl_event_source(loop, fd);
    }

    return nullptr;
}

int wl_event_loop_dispatch(wl_event_loop* loop, int timeout)
{
    return loop->event_loop->dispatch(timeout);
}

int wl_event_source_remove(wl_event_source* source)
{
    if (source->event_loop)
        source->event_loop->event_loop->remove_fd(source->fd);
    delete source;
    return 0;
}