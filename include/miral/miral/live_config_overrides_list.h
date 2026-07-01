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

#ifndef MIRAL_LIVE_CONFIG_OVERRIDES_LIST_H
#define MIRAL_LIVE_CONFIG_OVERRIDES_LIST_H

#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>

namespace miral::live_config
{
class OverridesListBuilder;

/// An ordered list of configuration file overrides to be applied. Each
/// callback represents a file that should be loaded into the configuration in
/// the given order (unchanged, fresh, modified, or dropped).
///
/// \remark Since MirAL 5.8
class OverridesList
{
    struct Context;
    friend OverridesListBuilder;

public:
    /// \pre The context must not be null
    explicit OverridesList(std::unique_ptr<Context> ctx);
    OverridesList(OverridesList&&) noexcept;
    OverridesList& operator=(OverridesList&&) noexcept;

    /// Callback type for file-content events (unchanged, moved/new, or modified).
    using Loader = std::move_only_function<void(std::filesystem::path const&, std::istream&)>;

    /// Callback type for file-removal events.
    using Dropped = std::move_only_function<void(std::filesystem::path const&)>;

    /// Iterate all events in push order, invoking the matching callback for each.
    void for_each(Loader unchanged, Loader fresh, Loader modified, Dropped dropped) const;

    ~OverridesList();

private:
    std::unique_ptr<Context> ctx;
};
}

#endif //MIRAL_LIVE_CONFIG_OVERRIDES_LIST_H
