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
#include "wayland_utils.h"
#include "wl_seat.h"

#include <mir_toolkit/event.h>

#include <atomic>
#include <functional>

struct WlSeat;
struct wl_client;
struct wl_resource;

namespace mir
{
namespace frontend
{

class BasicSurfaceEventSink : public NullEventSink
{
public:
    BasicSurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, std::shared_ptr<bool> const& destroyed)
        : seat{seat},
          client{client},
          target{target},
          window_size{geometry::Size{0,0}},
          destroyed{destroyed}
    {}

    void handle_event(MirEvent const& e) override;

    void latest_resize(geometry::Size window_size)
    {
        this->window_size = window_size;
    }

    auto latest_timestamp() const -> uint64_t
    {
        return timestamp;
    }

    template<typename Callable>
    inline void run_unless_destroyed(Callable&& callable) const
    {
        seat->spawn(run_unless(destroyed, callable));
    }

    virtual void send_resize(geometry::Size const& new_size) const = 0;

protected:
    WlSeat* const seat;
    wl_client* const client;
    wl_resource* const target;
    std::atomic<geometry::Size> window_size;
    std::atomic<int64_t> timestamp{0};
    std::shared_ptr<bool> const destroyed;
};
}
}

#endif //MIR_FRONTEND_BASIC_EVENT_SINK_H_
