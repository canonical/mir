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

#ifndef MIR_FRONTEND_PRIMARY_SELECTION_V1_H
#define MIR_FRONTEND_PRIMARY_SELECTION_V1_H

#include "wp_primary_selection_unstable_v1.h"

#include <mir/scene/data_exchange.h>

#include <memory>
#include <string>
#include <vector>

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
}
namespace frontend
{
class PrimarySelectionSourceV1 : public mir::wayland::PrimarySelectionSourceV1
{
public:
    class Source;

    PrimarySelectionSourceV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::PrimarySelectionSourceV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<mir::Executor> wayland_executor);

    auto offer(rust::String mime_type) -> void override;
    auto destroy() -> void override
    {
        destroy_and_delete();
    }
    auto make_source() -> std::shared_ptr<mir::scene::DataExchangeSource>;

private:
    std::vector<std::string> mime_types;
    std::shared_ptr<mir::Executor> const wayland_executor;
};

auto create_zwp_primary_selection_device_manager_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::PrimarySelectionDeviceManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::Clipboard> primary_selection_clipboard)
-> std::shared_ptr<wayland::PrimarySelectionDeviceManagerV1>;
}
}

#endif // MIR_FRONTEND_PRIMARY_SELECTION_V1_H
