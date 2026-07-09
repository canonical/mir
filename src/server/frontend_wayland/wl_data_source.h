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

#include "wayland.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

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
class WlDataSource : public wayland_rs::DataSource
{
public:
    WlDataSource(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::DataSourceMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> const& wayland_executor,
        scene::Clipboard& clipboard);
    ~WlDataSource() override;

    void set_clipboard_paste_source();
    void start_drag_n_drop_gesture();

    auto offer(rust::String mime_type) -> void override;
    auto set_actions(uint32_t dnd_actions) -> void override;
    auto destroy() -> void override
    {
        destroy_and_delete();
    }

    auto make_source() -> std::shared_ptr<mir::scene::DataExchangeSource>;

private:
    class ClipboardObserver;
    class Source;

    void paste_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);
    void drag_n_drop_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);

    auto drag_n_drop_set_actions(uint32_t dnd_actions, uint32_t preferred_action) -> uint32_t;

    std::shared_ptr<Executor> const wayland_executor;
    scene::Clipboard& clipboard;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    std::vector<std::string> mime_types;
    std::weak_ptr<scene::DataExchangeSource> paste_source;
    bool clipboards_paste_source_is_ours{false};

    std::weak_ptr<scene::DataExchangeSource> dnd_source;
    bool dnd_source_source_is_ours{false};
    uint32_t dnd_actions{0};
    std::optional<uint32_t> dnd_action;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_SOURCE_H_
