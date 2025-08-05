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

#ifndef MIRAL_MOUSE_KEYS_CONFIG_H
#define MIRAL_MOUSE_KEYS_CONFIG_H

#include "mir/input/mousekeys_keymap.h"

#include <memory>

namespace mir
{
class Server;
}

/// Enables configuring mousekeys at runtime.
///
/// Mousekeys is an accessibility feature that allows users to control the
/// pointer with their keyboard. The default keymap uses numpad `8`, `2`, `4`,
/// `6` for up, down, left, and right repsectively. Single clicks can be
/// dispatched with `5`, and double clicks with `+`. Holds can be initiated
/// with `0`, and releases with `.`. Users can switch between left, middle, and
/// right click with `/`, `*`, and `-` respectively.
///
/// \remark Since MirAL 5.3
/// \note All methods can only be called after the server is initialized.
namespace miral
{
namespace live_config
{
    class Store;
}
class MouseKeysConfig
{
public:
    [[deprecated(
        "MouseKeysConfig(bool) is deprecated. Please use MouseKeysConfig::enabled or MouseKeysConfig::disabled")]]
    explicit MouseKeysConfig(bool enabled_by_default);

    /// Creates a `MouseKeysConfig` instance that's enabled by default.
    /// \remark Since MirAL 5.5
    auto static enabled() -> MouseKeysConfig;

    /// Creates a `MouseKeysConfig` instance that's disabled by default.
    /// \remark Since MirAL 5.5
    auto static disabled() -> MouseKeysConfig;

    /// Construct a `MouseKeysConfig` instance with access to a live config store.
    ///
    /// Available options:
    ///     - {mouse_keys, enable}: Enable or disable mousekeys.
    ///     - {mouse_keys, acceleration, constant_factor}: The base speed for
    ///     mousekey pointer motion
    ///     - {mouse_keys, acceleration, linear_factor}: The linear speed
    ///     increase for mousekey pointer motion
    ///     - {mouse_keys, acceleration, quadratic_factor}: The quadratic speed
    ///     increase for mousekey pointer motion
    ///     - {mouse_keys, max_speed_x}: The maximum mousekeys speed on the x
    ///     axis
    ///     - {mouse_keys, max_speed_x}: The maximum mousekeys speed on the y
    ///     axis
    explicit MouseKeysConfig(live_config::Store& config_store);

    void operator()(mir::Server& server) const;

    /// Enables or disables mousekeys depending on the passed parameter.
    [[deprecated(
        "MouseKeysConfig::enabled(bool) is deprecated. Please use MouseKeysConfig::enable or MouseKeysConfig::disable")]]
    void enabled(bool enabled) const;

    /// Enables mousekeys.
    /// When already enabled, further calls have no effect.
    /// \remark Since MirAL 5.5
    void enable() const;
    
    /// Disables mousekeys.
    /// When already disabled, further calls have no effect.
    /// \remark Since MirAL 5.5
    void disable() const;

    /// Changes the keymap for the various mousekeys actions defined in
    /// `mir::input::MouseKeysKeymap::Action`.
    /// \note If a certain action not mapped to any key, it will be disabled.
    void set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const;

    /// Sets the factors used to accelerate the pointer during motion. Follows
    /// the equation: constant + linear * time + quadratic * time^2. Where time
    /// is the time since the cursor has started moving.
    /// \note The default acceleration factors are constant=100, linear=100,
    /// and quadratic=30
    void set_acceleration_factors(double constant, double linear, double quadratic) const;

    /// Sets the maximum speed in pixels/s for the pointer on the x and y axes
    /// respectively.
    /// \note The default maximum speed is x_axis=400 and y_axis=400
    void set_max_speed(double x_axis, double y_axis) const;

private:
    struct Self;
    MouseKeysConfig(std::shared_ptr<Self>);
    std::shared_ptr<Self> self;
};
}

#endif
