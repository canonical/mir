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

#ifndef MIRAL_STICKY_KEYS_H
#define MIRAL_STICKY_KEYS_H

#include <memory>
#include <functional>

namespace mir
{
class Server;
}

namespace miral
{
namespace live_config { class Store; }

/// Sticky keys is an accessibility feature that enables the user to
/// click a modifier key and have it "stick" until the keyboard shortcut is complete.
class StickyKeys
{
public:
    /// Creates a 'StickyKeys' instance that is enabled by default.
    static auto enabled() -> StickyKeys;

    // Creates a 'StickyKeys' instance that is disabled by default.
    static auto disabled() -> StickyKeys;

    /// Construct a 'StickyKeys' instance with access to a live config store.
    explicit StickyKeys(live_config::Store& config_store);

    void operator()(mir::Server& server);

    /// Enables sticky keys. Enabling while already enabled is idempotent.
    StickyKeys& enable();

    /// Disables sticky keys. Disabling while already disabled is idempotent.
    StickyKeys& disable();

    /// When set to true, depressing two modifier keys simultaneously will result
    /// in sticky keys being temporarily disabled until all keys are released.
    /// Defaults to true.
    StickyKeys& should_disable_if_two_keys_are_pressed_together(bool on);

    /// Configure a callback to be invoked whenever a modifier key is clicked
    /// and is now sticky pending an action that will release it.
    /// The integer argument is the scan code of the modifier, which you may
    /// match against those found in <linux/input-event-codes.h>.
    StickyKeys& on_modifier_clicked(std::function<void(int32_t)>&&);

private:
    class Self;
    explicit StickyKeys(std::shared_ptr<Self>);
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_STICKY_KEYS_H
