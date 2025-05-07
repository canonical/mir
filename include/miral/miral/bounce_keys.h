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

namespace mir
{
namespace input
{
class XkbSymKey;
}
class Server;
}

namespace miral
{
class BounceKeys
{
public:
    BounceKeys(bool enable_by_default);
    void operator()(mir::Server&);

    BounceKeys& delay(std::chrono::milliseconds);
    BounceKeys& on_press_rejected(std::function<void(unsigned int keysym)>&&);
    BounceKeys& on_enabled(std::function<void()>&&);
    BounceKeys& on_disabled(std::function<void()> &&);

    BounceKeys& enable();
    BounceKeys& disable();

private:
    struct Self;
    std::shared_ptr<Self> const self;
};
}

#endif // MIRAL_BOUNCE_KEYS_H
