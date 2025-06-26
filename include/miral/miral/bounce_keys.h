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

#include <chrono>
#include <functional>
#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Enables configuring bounce keys at runtime.
/// \remark Since MirAL 5.3
class BounceKeys
{
public:
    explicit BounceKeys(bool enable_by_default);
    void operator()(mir::Server&);

    /// Enables bounce keys.
    /// When already enabled, further calls have no effect.
    BounceKeys& enable();

    /// Disables bounce keys.
    /// When already disabled, further calls have no effect.
    BounceKeys& disable();

    /// Configures the duration users have to hold a key down for the press to register.
    BounceKeys& delay(std::chrono::milliseconds);

    /// Configures the callback to invoke when a keypress is rejected. Can be
    /// used to provide audio feedback or to initialize animations for
    /// graphical feedback.
    BounceKeys& on_press_rejected(std::function<void(unsigned int keysym)>&&);

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif // MIRAL_BOUNCE_KEYS_H
