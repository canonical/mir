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

#ifndef MIR_FRONTEND_WL_TOUCH_H
#define MIR_FRONTEND_WL_TOUCH_H

#include "generated/wayland_wrapper.h"

#include "mir/geometry/point.h"

#include <map>
#include <functional>

// from "mir_toolkit/events/event.h"
struct MirTouchEvent;

namespace mir
{
class Executor;

namespace frontend
{
class WlSurface;

class WlTouch : public wayland::Touch
{
public:
    WlTouch(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlTouch*)> const& on_destroy);

    ~WlTouch();

    void handle_event(MirTouchEvent const* touch_ev, WlSurface* surface);

private:
    std::function<void(WlTouch*)> on_destroy;
    std::map<int32_t, WlSurface*> focused_surface_for_ids;

    void handle_down(mir::geometry::Point position, WlSurface* surface, uint32_t time, int32_t id);
    void handle_up(uint32_t time, int32_t id);

    void release() override;
};

}
}

#endif // MIR_FRONTEND_WL_TOUCH_H
