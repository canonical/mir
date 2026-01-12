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
#include <map>
#include <ranges>

// In the Rust version of wayland-server, the event
// loop is handled in the display.
struct wl_event_loop
{
    EventLoop* event_loop; // Raw pointer  to event loop, may be owned or borrowed
    std::optional<rust::Box<EventLoop>> owned_loop; // Owned reference, only set when event loops are manually created.

    std::map<uint64_t, wl_listener*> destroy_listeners;
};

struct wl_display
{
    rust::Box<WaylandServer> wrapper;
    wl_event_loop event_loop; // Embedded event loop that borrows from wrapper
    std::map<uint64_t, wl_listener*> destroy_listeners;
};

struct wl_event_source
{
    wl_event_loop* event_loop;
    uint64_t source;
};

wl_display* wl_display_create()
{
    auto result = new wl_display
    {
        create_wayland_server(),
        wl_event_loop{},
        {}
    };
    
    // Initialize the embedded event loop to borrow from the display wrapper
    result->event_loop.event_loop = &result->wrapper->get_eventloop();
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
    // TODO
    (void)(display);
    (void)(name);
}

void wl_display_terminate(wl_display* display)
{
    display->event_loop.event_loop->terminate();
}

void wl_display_run(wl_display* display)
{
    display->wrapper->run();
}

void wl_display_flush_clients(wl_display* display)
{
    display->wrapper->flush_clients();
}

void wl_display_destroy_clients(wl_display* display)
{
    // TODO: noop in rust
    (void)display;
}

uint32_t wl_display_next_serial(wl_display* display)
{
    return display->wrapper->next_serial();
}

wl_event_loop* wl_event_loop_create()
{
    auto owned = create_event_loop();
    return new wl_event_loop{owned.into_raw(), std::move(owned), {}};
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
    uint64_t source = loop->event_loop->add_fd(
        fd,
        mask,
        reinterpret_cast<size_t>(func),
        reinterpret_cast<size_t>(data)
    );
    if (source > 0)
    {
        return new wl_event_source(loop, source);
    }

    return nullptr;
}

int wl_event_loop_dispatch(wl_event_loop* loop, int timeout)
{
    return loop->event_loop->dispatch(timeout);
}

int wl_event_loop_get_fd(wl_event_loop* loop)
{
    return loop->event_loop->get_fd();
}

// Wrapper callback that Rust will call, which then calls the wl_listener's notify
static void destroy_listener_wrapper(void* data)
{
    auto* context = static_cast<std::pair<wl_event_loop*, wl_listener*>*>(data);
    auto* loop = context->first;
    auto* listener = context->second;
    
    // Call the listener's notify function with the listener and loop as data
    listener->notify(listener, loop);
    
    delete context;
}

void wl_event_loop_add_destroy_listener(
    wl_event_loop *loop,
    wl_listener *listener)
{
    // Create context to pass both loop and listener to the wrapper
    auto* context = new std::pair<wl_event_loop*, wl_listener*>(loop, listener);
    
    // Register with Rust event loop
    uint64_t id = loop->event_loop->add_destroy_listener(
        reinterpret_cast<size_t>(&destroy_listener_wrapper),
        reinterpret_cast<size_t>(context)
    );
    
    // Store the listener so we can find it later
    loop->destroy_listeners[id] = listener;
}

wl_listener* wl_event_loop_get_destroy_listener(
    wl_event_loop *loop,
    wl_notify_func_t notify)
{
    // Search for a listener with the matching notify function
    for (const auto& listener: loop->destroy_listeners | std::views::values)
    {
        if (listener->notify == notify)
        {
            return listener;
        }
    }
    return nullptr;
}

// Wrapper callback that Rust will call for display destroy, which then calls the wl_listener's notify
static void display_destroy_listener_wrapper(void* data)
{
    auto* context = static_cast<std::pair<wl_display*, wl_listener*>*>(data);
    auto* display = context->first;
    auto* listener = context->second;
    
    // Call the listener's notify function with the listener and display as data
    listener->notify(listener, display);
    
    delete context;
}

void wl_display_add_destroy_listener(
    wl_display *display,
    wl_listener *listener)
{
    // Create context to pass both display and listener to the wrapper
    auto* context = new std::pair<wl_display*, wl_listener*>(display, listener);
    
    // Register with Rust display wrapper
    uint64_t id = display->wrapper->add_destroy_listener(
        reinterpret_cast<size_t>(&display_destroy_listener_wrapper),
        reinterpret_cast<size_t>(context)
    );
    
    // Store the listener so we can find it later
    display->destroy_listeners[id] = listener;
}

int wl_event_source_remove(wl_event_source* source)
{
    if (source->event_loop)
        source->event_loop->event_loop->remove_source(source->source);
    delete source;
    return 0;
}
