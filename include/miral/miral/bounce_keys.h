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

#ifndef MIRAL_BOUNCE_KEYS_H
#define MIRAL_BOUNCE_KEYS_H

#include <miral/toolkit_event.h>

#include <chrono>
#include <functional>
#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }
/// Enables configuring bounce keys at runtime.
///
/// Bounce keys is an accessibility feature that enables rejection of
/// keypresses of the same key that occur within a certain delay of each other.
/// Optionally, you (the shell author) can attach a rejection handler to
/// provide feedback to users.
///
/// \remark Since MirAL 5.5
class BounceKeys
{
public:
    /// Creates a `BounceKeys` instance that's enabled by default.
    static auto enabled() -> BounceKeys;

    /// Creates a `BounceKeys` instance that's disabled by default.
    static auto disabled() -> BounceKeys;

    /// Construct a `BounceKeys` instance with access to a live config store.
    ///
    /// Available options:
    ///     - {bounce_keys, enable}: Enable or disable bounce keys.
    ///     - {bounce_keys, delay}: How much time in milliseconds must pass
    ///     between keypresses to not be rejected.
    explicit BounceKeys(live_config::Store& config_store);

    void operator()(mir::Server&);

    /// Enables bounce keys.
    /// When already enabled, further calls have no effect.
    BounceKeys& enable();

    /// Disables bounce keys.
    /// When already disabled, further calls have no effect.
    BounceKeys& disable();

    /// Configures the duration users have to hold a key down for the press to
    /// register.
    /// \note The default delay period is 200 milliseconds.
    BounceKeys& delay(std::chrono::milliseconds);

    /// Configures the callback to invoke when a keypress is rejected. Can be
    /// used to provide audio feedback or to initialize animations for
    /// graphical feedback.
    BounceKeys& on_press_rejected(std::function<void(MirKeyboardEvent const*)>&&);

private:
    struct Self;
    BounceKeys(std::shared_ptr<Self> self);
    std::shared_ptr<Self> const self;
};
}

#endif // MIRAL_BOUNCE_KEYS_H
