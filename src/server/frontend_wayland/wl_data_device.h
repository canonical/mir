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
class WlDataDevice : public wayland::DataDevice, public WlSeat::FocusListener
{
public:
    WlDataDevice(
        wl_resource* new_resource,
        Executor& wayland_executor,
        scene::Clipboard& clipboard,
        WlSeat& seat);
    ~WlDataDevice();

    /// Wayland requests
    /// @{
    void start_drag(
        std::optional<wl_resource*> const& source, wl_resource* origin,
        std::optional<wl_resource*> const& icon, uint32_t serial) override
    {
        (void)source, (void)origin, (void)icon, (void)serial;
    }
    void set_selection(std::optional<wl_resource*> const& source, uint32_t serial) override;
    /// @}

private:
    class ClipboardObserver;
    class Offer;

    /// Override from WlSeat::FocusListener
    void focus_on(WlSurface* surface) override;

    /// Called by the clipboard observer
    void paste_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);

    scene::Clipboard& clipboard;
    WlSeat& seat;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    bool has_focus = false;
    wayland::Weak<Offer> current_offer;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_H_
