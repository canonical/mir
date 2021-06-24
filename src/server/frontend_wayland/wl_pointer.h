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

#include <chrono>
#include <functional>
#include <optional>
#include <set>

struct MirInputEvent;
typedef unsigned int MirPointerButtons;

struct MirPointerEvent;

namespace mir
{
namespace wayland { class RelativePointerV1; }

class Executor;

namespace frontend
{
class WlSurface;

class CommitHandler
{
public:
    virtual void on_commit(WlSurface* surface) = 0;

protected:
    CommitHandler() = default;
    ~CommitHandler() = default;
    CommitHandler(CommitHandler const&) = delete;
    CommitHandler& operator=(CommitHandler const&) = delete;
};

class WlPointer : public wayland::Pointer, private CommitHandler
{
public:

    WlPointer(wl_resource* new_resource);

    ~WlPointer();

    void set_relative_pointer(wayland::RelativePointerV1* relative_ptr);

    /// Convert the Mir event into Wayland events and send them to the client. root_surface is the one that received
    /// the Mir event, but the final Wayland event may be sent to a subsurface.
    void event(MirPointerEvent const* event, WlSurface& root_surface);

    struct Cursor;

private:
    wl_display* const display;

    void leave(std::optional<MirPointerEvent const*> event);
    void buttons(MirPointerEvent const* event);
    void axis(MirPointerEvent const* event);
    /// Handles finding the correct subsurface and position on that subsurface if needed
    /// Giving it an already transformed surface and position is also fine
    void enter_or_motion(MirPointerEvent const* event, WlSurface& root_surface);
    /// Sends relative motion only if the relative pointer is set
    void relative_motion(MirPointerEvent const* event);
    /// Sends a frame event only if needed, leaves needs_frame false
    void maybe_frame();
    /// The cursor surface has committed
    void on_commit(WlSurface* surface) override;

    /// Wayland request handlers
    ///@{
    void set_cursor(
        uint32_t serial,
        std::experimental::optional<wl_resource*> const& surface,
        int32_t hotspot_x,
        int32_t hotspot_y) override;
    ///@}

    wayland::Weak<WlSurface> surface_under_cursor;
    wayland::DestroyListenerId destroy_listener_id; ///< ID of this pointer's destroy listener on surface_under_cursor
    bool needs_frame{false};
    MirPointerButtons current_buttons{0};
    std::experimental::optional<std::pair<float, float>> current_position;
    std::unique_ptr<Cursor> cursor;
    wayland::Weak<wayland::RelativePointerV1> relative_pointer;
    geometry::Displacement cursor_hotspot;
};

}
}

#endif // MIR_FRONTEND_WL_POINTER_H
