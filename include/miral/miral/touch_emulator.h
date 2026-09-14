/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIRAL_TOUCH_EMULATOR_H
#define MIRAL_TOUCH_EMULATOR_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Enables or disables the built-in touch emulation in the Mir server.
///
/// When enabled, a compositor can synthesize touch events from mouse input,
/// allowing touch-enabled applications to be tested without a touch screen.
///
/// \remark Since MirAL 6.0
class TouchEmulator
{
public:
    /// Creates a TouchEmulator that is enabled by default.
    ///
    /// \returns an enabled TouchEmulator
    static auto enabled() -> TouchEmulator;

    /// Creates a TouchEmulator that is disabled by default.
    ///
    /// \returns a disabled TouchEmulator
    static auto disabled() -> TouchEmulator;

    /// Enables touch emulation.
    ///
    /// \returns a reference to this TouchEmulator
    auto enable() -> TouchEmulator&;

    /// Disables touch emulation.
    ///
    /// \returns a reference to this TouchEmulator
    auto disable() -> TouchEmulator&;

    /// Construct a `TouchEmulator` instance with access to a live config store.
    ///
    /// Available options:
    ///     - {touch-emulator, enable}: Enable or disable touch emulation.
    ///
    /// \param config_store the configuration store used to persist the touch emulation state
    explicit TouchEmulator(miral::live_config::Store& config_store);

    /// Applies the touch emulator configuration to the Mir \p server.
    void operator()(mir::Server& server);

private:
    class Self;
    explicit TouchEmulator(std::shared_ptr<Self> self);
    std::shared_ptr<Self> self;
};
}

#endif
