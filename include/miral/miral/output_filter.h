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

#ifndef MIRAL_OUTPUT_FILTER_H
#define MIRAL_OUTPUT_FILTER_H

#include <mir_toolkit/common.h>

#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Allows for the configuration of the output filter at runtime.
/// \remark Since MirAL 5.3
class OutputFilter
{
public:
    explicit OutputFilter();

    /// Construct registering with a configuration store
    /// \remark Since Miral 5.4
    explicit OutputFilter(live_config::Store& config_store);
    explicit OutputFilter(MirOutputFilter default_filter);
    ~OutputFilter();

    /// Applies the new filter. (Either immediately or when the server starts)
    void filter(MirOutputFilter new_filter) const;

    void operator()(mir::Server& server) const;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
