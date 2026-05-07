/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_LIVE_CONFIG_OVERRIDES_LIST_BUILDER_H
#define MIR_LIVE_CONFIG_OVERRIDES_LIST_BUILDER_H

#include <miral/live_config_overrides_list.h>

namespace miral::live_config
{
struct OverridesList::Context
{
    enum class Kind { unchanged, fresh, modified, dropped };

    struct Event
    {
        Event(Kind kind, std::filesystem::path path, std::function<std::unique_ptr<std::istream>()> stream_factory);

        Kind const kind;
        std::filesystem::path const path;
        std::function<void(OverridesList::Loader &)> const file_opener;
    };

    std::vector<Event> events;
};

class OverridesListBuilder
{
public:
    auto push_unchanged(
        std::filesystem::path filepath,
        std::function<std::unique_ptr<std::istream>()> stream) -> OverridesListBuilder&;

    auto push_new(
        std::filesystem::path filepath,
        std::function<std::unique_ptr<std::istream>()> stream) -> OverridesListBuilder&;

    auto push_modified(
        std::filesystem::path filepath,
        std::function<std::unique_ptr<std::istream>()> stream) -> OverridesListBuilder&;

    auto push_dropped(std::filesystem::path filepath) -> OverridesListBuilder&;

    auto build() -> OverridesList;
private:
    using Context = OverridesList::Context;
    std::unique_ptr<Context> ctx = std::make_unique<Context>();
};
}

#endif //MIR_LIVE_CONFIG_OVERRIDES_LIST_BUILDER_H
