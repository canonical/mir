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

#ifndef MIR_FRONTEND_WL_DATA_SOURCE_H_
#define MIR_FRONTEND_WL_DATA_SOURCE_H_

#include "wayland_wrapper.h"
#include "wl_surface.h"

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
class DragIconController;
class PointerInputDispatcher;

class WlDataSource : public wayland::DataSource
{
public:
    WlDataSource(
        wl_resource* new_resource,
        std::shared_ptr<Executor> const& wayland_executor,
        scene::Clipboard& clipboard,
        std::shared_ptr<DragIconController> drag_icon_controller,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher);
    ~WlDataSource();

    static auto from(struct wl_resource* resource) -> WlDataSource*;

    void set_clipboard_paste_source();
    void start_drag_n_drop_gesture(std::optional<wl_resource*> const& icon);

    /// Wayland requests
    /// @{
    void offer(std::string const& mime_type) override;
    void set_actions(uint32_t dnd_actions) override;
    /// @}

private:
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
    class ClipboardObserver;
    class Source;

    void paste_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);
    void drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);
    void drag_n_drop_source_cleared(std::shared_ptr<scene::DataExchangeSource> const& source);
    void end_drag_n_drop_gesture();
    uint32_t drag_n_drop_set_actions(uint32_t dnd_actions, uint32_t preferred_action);

    std::shared_ptr<Executor> const wayland_executor;
    scene::Clipboard& clipboard;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    std::shared_ptr<DragIconController> const drag_icon_controller;
    std::shared_ptr<PointerInputDispatcher> const pointer_input_dispatcher;
    std::vector<std::string> mime_types;
    std::weak_ptr<scene::DataExchangeSource> paste_source;
    bool clipboards_paste_source_is_ours{false};

    std::weak_ptr<scene::DataExchangeSource> dnd_source;
    bool dnd_source_source_is_ours{false};
    uint32_t dnd_actions;
    std::optional<DragIconSurface> drag_surface;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_SOURCE_H_
