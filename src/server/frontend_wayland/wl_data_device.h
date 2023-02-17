/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_WL_DATA_DEVICE_H_
#define MIR_FRONTEND_WL_DATA_DEVICE_H_

#include "mir/events/input_event.h"
#include "mir/executor.h"
#include "mir/input/event_filter.h"
#include "mir/scene/surface.h"
#include "wayland_wrapper.h"
#include "wl_seat.h"
#include "wl_surface.h"

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
class ClipboardSource;
}

namespace input
{
class CompositeEventFilter;
}

namespace frontend
{
class DragWlSurface : public NullWlSurfaceRole
{
public:
    DragWlSurface(WlSurface* icon);
    ~DragWlSurface();

    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;
    void create_scene_surface();
    void surface_destroyed() override;

private:
    wayland::Weak<WlSurface> const surface;
    std::shared_ptr<scene::Surface> shared_scene_surface;
};

class WlDataDevice : public wayland::DataDevice, public WlSeat::FocusListener
{
public:
    WlDataDevice(
        wl_resource* new_resource,
        Executor& wayland_executor,
        scene::Clipboard& clipboard,
        WlSeat& seat,
        input::CompositeEventFilter& composite_event_filter);
    ~WlDataDevice();

    /// Wayland requests
    /// @{
    void start_drag(
        std::optional<wl_resource*> const& source,
        wl_resource* origin,
        std::optional<wl_resource*> const& icon,
        uint32_t serial) override;

    void set_selection(std::optional<wl_resource*> const& source, uint32_t serial) override;
    /// @}

    void end_drag();

private:
    class ClipboardObserver;
    class CursorEventFilter;
    class Offer;

    /// Override from WlSeat::FocusListener
    void focus_on(WlSurface* surface) override;

    /// Called by the clipboard observer
    void paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source);

    scene::Clipboard& clipboard;
    WlSeat& seat;
    input::CompositeEventFilter& composite_event_filter;
    std::shared_ptr<CursorEventFilter> cursor_event_filter;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    bool has_focus = false;
    wayland::Weak<Offer> current_offer;
    std::optional<DragWlSurface> drag_surface;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_H_
