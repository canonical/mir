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

#ifndef MIRAL_ENABLE_MOUSEKEYS_H
#define MIRAL_ENABLE_MOUSEKEYS_H

#include "mir/input/mousekeys_common.h"

#include <memory>

namespace mir
{
class Server;
namespace input
{
class MouseKeysKeymap;
}
}

/// Enables configuring mousekeys at runtime.
/// \remark Since MirAL 5.3
/// \note All methods can only be called after the server is initialized.
namespace miral
{
class MouseKeysConfig
{
public:
    /// \note `--enable-mouse-keys` has higher precedence than
    /// [enabled_by_default]
    explicit MouseKeysConfig(bool enabled_by_default);
    void operator()(mir::Server& server) const;

    /// Enables or disables mousekeys depending on the passed parameter.
    void set_mousekeys_enabled(bool enabled) const;

    /// Changes the keymap for the various mousekeys actions defined in
    /// [MouseKeysKeymap::Action]
    /// \note If a certain action not mapped to any key, it will be disabled.
    void set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const;

    /// Sets the factors used to accelerate the pointer during motion. Follows
    /// the equation: constant + linear * time + quadratic * time^2. Where time
    /// is the time since the cursor has started moving.
    void set_acceleration_factors(double constant, double linear, double quadratic) const;

    /// Sets the maximum speed in pixels/s for the pointer on the x and y axes
    /// respectively.
    void set_max_speed(double x_axis, double y_axis) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
