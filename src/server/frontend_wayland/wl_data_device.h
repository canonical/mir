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

#include "wayland_wrapper.h"
#include "wl_seat.h"
#include "wl_surface.h"

#include <mir/events/event.h>

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
class DataExchangeSource;
}

namespace frontend
{
class PointerInputDispatcher;
class DragIconController;

class WlDataDevice : public wayland::DataDevice, public WlSeat::FocusListener
{
public:
    WlDataDevice(
        wl_resource* new_resource,
        Executor& wayland_executor,
        scene::Clipboard& clipboard,
        WlSeat& seat,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
        std::shared_ptr<DragIconController> drag_icon_controller);
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

    void event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface);

private:
    class ClipboardObserver;
    class Offer;
    class DragIconSurface : public NullWlSurfaceRole
    {
    public:
        DragIconSurface(WlSurface* icon, std::shared_ptr<DragIconController> drag_icon_controller);
        ~DragIconSurface();

        auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;

    private:
        wayland::Weak<WlSurface> const surface;
        std::shared_ptr<scene::Surface> shared_scene_surface;
        std::shared_ptr<DragIconController> const drag_icon_controller;
    };

    /// Override from WlSeat::FocusListener
    void focus_on(WlSurface* surface) override;

    /// Called by the clipboard observer
    void paste_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);
    void drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);
    void drag_n_drop_source_cleared();

    void validate_pointer_event(std::optional<std::shared_ptr<MirEvent const>> drag_event) const;
    void make_new_dnd_offer_if_possible(std::shared_ptr<mir::scene::DataExchangeSource> const& source);

    void end_of_dnd_gesture();

    scene::Clipboard& clipboard;
    WlSeat& seat;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    std::shared_ptr<PointerInputDispatcher> const pointer_input_dispatcher;
    std::shared_ptr<DragIconController> const drag_icon_controller;
    std::function<void()> const end_of_gesture_callback;
    bool has_focus = false;
    std::weak_ptr<scene::DataExchangeSource> weak_source;
    wayland::Weak<Offer> current_offer;
    wayland::Weak<WlSurface> weak_surface;
    bool sent_enter = false;
    std::optional<DragIconSurface> drag_surface;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_H_
