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

#include "mir/scene/data_exchange.h"
#include "primary-selection-unstable-v1_wrapper.h"

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
}
namespace frontend
{
class PrimarySelectionSource : public mir::wayland::PrimarySelectionSourceV1
{
private:
    std::vector<std::string> mime_types;
    std::shared_ptr<mir::Executor> const wayland_executor;

public:
    class Source;

    PrimarySelectionSource(wl_resource* resource, std::shared_ptr<mir::Executor> wayland_executor);

    static auto from(struct wl_resource* resource) -> PrimarySelectionSource*;

    void offer(std::string const& mime_type) override;
    auto make_source() const -> std::shared_ptr<mir::scene::DataExchangeSource>;
};
auto create_primary_selection_device_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::Clipboard> primary_selection_clipboard)
-> std::shared_ptr<wayland::PrimarySelectionDeviceManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_PRIMARY_SELECTION_V1_H
