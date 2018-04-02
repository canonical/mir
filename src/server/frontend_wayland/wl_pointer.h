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

#ifndef MIR_FRONTEND_WL_POINTER_H
#define MIR_FRONTEND_WL_POINTER_H

#include "mir/geometry/point.h"

#include "generated/wayland_wrapper.h"

struct MirInputEvent;
typedef unsigned int MirPointerButtons;

namespace mir
{

class Executor;

namespace frontend
{

class WlPointer : public wayland::Pointer
{
public:

    WlPointer(
        wl_client* client,
        wl_resource* parent,
        uint32_t id,
        std::function<void(WlPointer*)> const& on_destroy,
        std::shared_ptr<mir::Executor> const& executor);

    ~WlPointer();

    void handle_event(MirInputEvent const* event, wl_resource* target);

    void handle_button(uint32_t time, uint32_t button, wl_pointer_button_state state);
    void handle_enter(mir::geometry::Point position, wl_resource* target);
    void handle_motion(uint32_t time, mir::geometry::Point position);
    void handle_axis(uint32_t time, wl_pointer_axis axis, double distance);
    void handle_leave(wl_resource* target);

    struct Cursor;

private:
    wl_display* const display;
    std::shared_ptr<mir::Executor> const executor;

    std::function<void(WlPointer*)> on_destroy;
    std::shared_ptr<bool> const destroyed;

    MirPointerButtons last_set{0};
    float last_x{0}, last_y{0};

    void set_cursor(uint32_t serial, std::experimental::optional<wl_resource*> const& surface, int32_t hotspot_x, int32_t hotspot_y) override;
    void release() override;

    std::unique_ptr<Cursor> cursor;
};

}
}

#endif // MIR_FRONTEND_WL_POINTER_H
