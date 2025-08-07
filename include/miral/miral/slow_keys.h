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

#ifndef MIRAL_SLOW_KEYS_H
#define MIRAL_SLOW_KEYS_H

#include <chrono>
#include <functional>

namespace mir
{
class Server;
}

namespace miral
{
namespace live_config { class Store; }

/// Enables configuring slow keys at runtime.
///
/// Slow keys is an accessibility feature that enables the rejection of
/// keypresses that don't last long enough. It can be useful in cases where the
/// user has issues that cause them to press buttons accidentally.
///
/// You can optionally assign handlers for when the key is pressed down, is
/// rejected, or when the press is accepted.
///
/// \remark  Since MirAL 5.5
class SlowKeys
{
public:
    /// Creates a `SlowKeys` instance that's enabled by default.
    static auto enabled() -> SlowKeys;

    /// Creates a `SlowKeys` instance that's disabled by default.
    static auto disabled() -> SlowKeys;

    /// Construct a `SlowKeys` instance with access to a live config store.
    explicit SlowKeys(miral::live_config::Store& config_store);

    void operator()(mir::Server& server);

    // Enables slow keys.
    // When already enabled, further calls have no effect.
    SlowKeys& enable();
    
    // Disables slow keys.
    // When already disabled, further calls have no effect.
    SlowKeys& disable();

    /// Configures the duration a key has to be pressed down for to register as
    /// a key press.
    /// \note The default hold delay is 200 milliseconds.
    SlowKeys& hold_delay(std::chrono::milliseconds hold_delay);

    /// Configures the callback that's invoked when the key is pressed down.
    /// Useful for providing feedback to users.
    SlowKeys& on_key_down(std::function<void(unsigned int)>&& on_key_down);
    
    /// Configures the callback that's invoked when a press is rejected.
    /// Useful for providing feedback to users.
    SlowKeys& on_key_rejected(std::function<void(unsigned int)>&& on_key_rejected);
    
    /// Configures the callback that's invoked when a press is accepted.
    /// Useful for providing feedback to users.
    SlowKeys& on_key_accepted(std::function<void(unsigned int)>&& on_key_accepted);

private:
    struct Self;
    explicit SlowKeys(std::shared_ptr<Self>);
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SLOW_KEYS_H
