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

#ifndef MIRAL_CURSOR_SCALE_H
#define MIRAL_CURSOR_SCALE_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Allows for the configuration of the cursor's scale at runtime.
/// \remark Since MirAL 5.3
class CursorScale
{
public:
    explicit CursorScale();

    /// Construct registering with a configuration store
    /// \remark Since Miral 5.5
    explicit CursorScale(live_config::Store& config_store);
    explicit CursorScale(float default_scale);
    ~CursorScale();

    /// Applies the new scale. (Either immediately or when the server starts)
    void scale(float new_scale) const;

    void operator()(mir::Server& server) const;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
