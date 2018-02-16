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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_BASIC_EVENT_SINK_H_
#define MIR_FRONTEND_BASIC_EVENT_SINK_H_

#include "null_event_sink.h"

#include <mir_toolkit/event.h>

#include <wayland-server-core.h>

#include <atomic>
#include <functional>

namespace mir
{
namespace frontend
{
class BasicWlSeat
{
public:
    virtual ~BasicWlSeat() = default;

    virtual void handle_pointer_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const = 0;
    virtual void handle_keyboard_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const = 0;
    virtual void handle_touch_event(wl_client* client, MirInputEvent const* input_event, wl_resource* target) const = 0;
    virtual void handle_event(wl_client* client, MirKeymapEvent const* keymap_event, wl_resource* target) const = 0;
    virtual void handle_event(wl_client* client, MirWindowEvent const* window_event, wl_resource* target) const = 0;
    virtual void spawn(std::function<void()>&& work) = 0;
};

class BasicSurfaceEventSink : public NullEventSink
{
public:
    BasicSurfaceEventSink(BasicWlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink)
        : seat{seat},
        client{client},
        target{target},
        event_sink{event_sink},
        window_size{geometry::Size{0,0}}
    {
    }

    void handle_event(MirEvent const& e) override;

    void latest_resize(geometry::Size window_size)
    {
        this->window_size = window_size;
    }

    auto latest_timestamp() const -> uint64_t
    {
        return timestamp;
    }

    virtual void send_resize(geometry::Size const& new_size) const = 0;

protected:
    BasicWlSeat* const seat;
    wl_client* const client;
    wl_resource* const target;
    wl_resource* const event_sink;
    std::atomic<geometry::Size> window_size;
    std::atomic<int64_t> timestamp{0};
};
}
}

#endif //MIR_FRONTEND_BASIC_EVENT_SINK_H_
