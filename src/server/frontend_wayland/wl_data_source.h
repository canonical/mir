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

#ifndef MIR_FRONTEND_WL_DATA_SOURCE_H_
#define MIR_FRONTEND_WL_DATA_SOURCE_H_

#include "wayland_wrapper.h"

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
class ClipboardSource;
}

namespace frontend
{
class WlDataSource : public wayland::DataSource
{
public:
    WlDataSource(
        wl_resource* new_resource,
        std::shared_ptr<Executor> const& wayland_executor,
        scene::Clipboard& clipboard);
    ~WlDataSource();

    static auto from(struct wl_resource* resource) -> WlDataSource*;

    void set_clipboard_paste_source();

    /// Wayland requests
    /// @{
    void offer(std::string const& mime_type) override;
    void set_actions(uint32_t dnd_actions) override
    {
        (void)dnd_actions;
    }
    /// @}

private:
    class ClipboardObserver;
    class Source;

    void paste_source_set(std::shared_ptr<scene::ClipboardSource> const& source);

    std::shared_ptr<Executor> const wayland_executor;
    scene::Clipboard& clipboard;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    std::vector<std::string> mime_types;
    std::weak_ptr<scene::ClipboardSource> paste_source;
    bool clipboards_paste_source_is_ours{false};
};
}
}

#endif // MIR_FRONTEND_WL_DATA_SOURCE_H_
