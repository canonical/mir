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

#include <mir/scene/data_exchange.h>
#include "wp_primary_selection_unstable_v1.h"
#include "client.h"

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
}
namespace frontend
{
class PrimarySelectionSource : public wayland_rs::ZwpPrimarySelectionSourceV1Impl, public std::enable_shared_from_this<PrimarySelectionSource>
{
public:
    class Source;

    PrimarySelectionSource(std::shared_ptr<mir::Executor> wayland_executor);

    static auto from(ZwpPrimarySelectionSourceV1Impl* resource) -> PrimarySelectionSource*;
    void offer(rust::String mime_type) override;
    auto make_source() const -> std::shared_ptr<mir::scene::DataExchangeSource>;

private:
    std::vector<std::string> mime_types;
    std::shared_ptr<mir::Executor> const wayland_executor;
};

class PrimarySelectionManager : public wayland_rs::ZwpPrimarySelectionDeviceManagerV1Impl
{
public:
    PrimarySelectionManager(
        std::shared_ptr<wayland_rs::Client> const& client,
        std::shared_ptr<mir::Executor> wayland_executor,
        std::shared_ptr<scene::Clipboard> primary_selection_clipboard);
    auto create_source() -> std::shared_ptr<wayland_rs::ZwpPrimarySelectionSourceV1Impl> override;
    auto get_device(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat)
        -> std::shared_ptr<wayland_rs::ZwpPrimarySelectionDeviceV1Impl> override;

    std::shared_ptr<wayland_rs::Client> const& client;
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<scene::Clipboard> const primary_selection_clipboard;
};

}
}

#endif // MIR_FRONTEND_PRIMARY_SELECTION_V1_H
