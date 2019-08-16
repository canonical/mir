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


#include "wayland_wrapper.h"

#include "mir/geometry/point.h"
#include "mir/geometry/displacement.h"

#include <functional>
#include <chrono>

struct MirInputEvent;
typedef unsigned int MirPointerButtons;

struct MirPointerEvent;

namespace mir
{

class Executor;

namespace frontend
{
class WlSurface;

class WlPointer : public wayland::Pointer
{
public:

    WlPointer(
        wl_resource* new_resource,
        std::function<void(WlPointer*)> const& on_destroy);

    ~WlPointer();

    void handle_event(MirPointerEvent const* event, WlSurface* surface);

    /// Handles finding the correct subsurface and position on that subsurface if needed
    /// Giving it an already transformed surface and position is also fine
    void enter(WlSurface* parent_surface, geometry::Point const& position_on_parent);
    void leave();
    void button(std::chrono::milliseconds const& ms, uint32_t button, bool pressed);
    void motion(
        std::chrono::milliseconds const& ms,
        WlSurface* parent_surface,
        geometry::Point const& position_on_parent);
    void axis(std::chrono::milliseconds const& ms, geometry::Displacement const& scroll);
    void frame();

    struct Cursor;

private:
    wl_display* const display;
    std::function<void(WlPointer*)> on_destroy;

    MirPointerButtons last_buttons{0};
    bool can_send_frame{false};
    std::experimental::optional<WlSurface*> focused_surface;

    /// Wayland request handlers
    ///@{
    void set_cursor(
        uint32_t serial,
        std::experimental::optional<wl_resource*> const& surface,
        int32_t hotspot_x,
        int32_t hotspot_y) override;
    void release() override;
    ///@}

    std::unique_ptr<Cursor> cursor;
};

}
}

#endif // MIR_FRONTEND_WL_POINTER_H
