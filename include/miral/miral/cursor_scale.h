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

#include <chrono>
#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Allows for the configuration of the cursor's scale at runtime.
///
/// \remark Since MirAL 5.3
class CursorScale
{
public:
    /// Construct a cursor scale with a scale of 1 by default.
    explicit CursorScale();

    /// Construct registering with a configuration store
    ///
    /// Available options:
    ///     - {cursor, scale}: Scales the cursor by the provided scale. Must be
    ///     between 0 and 100 inclusive.
    ///
    /// \param config_store the config store
    /// \remark Since Miral 5.5
    explicit CursorScale(live_config::Store& config_store);

    /// Construct a cursor scale with a default scale.
    ///
    /// The scale is clamped between 0 and 100, inclusive.
    ///
    /// \param default_scale the default scale
    explicit CursorScale(float default_scale);
    ~CursorScale();

    /// Applies a new scale, either immediately or when the server starts.
    ///
    /// The scale is clamped between 0 and 100, inclusive.
    ///
    /// \param new_scale the new scale
    void scale(float new_scale) const;

    /// Temporarily scales the cursor up or down by the given multiplier.
    /// \note This method is added as a temporary helper and may be removed at any time
    void scale_temporarily(float scale_multiplier, std::chrono::milliseconds duration) const;

    void operator()(mir::Server& server) const;
private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
