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
class SlowKeys
{
public:
    static auto enabled() -> SlowKeys;
    static auto disabled() -> SlowKeys;

    void operator()(mir::Server& server);

    SlowKeys& enable();
    SlowKeys& disable();
    SlowKeys& hold_delay(std::chrono::milliseconds hold_delay);
    SlowKeys& on_key_down(std::function<void(unsigned int)>&& on_key_down);
    SlowKeys& on_key_rejected(std::function<void(unsigned int)>&& on_key_rejected);
    SlowKeys& on_key_accepted(std::function<void(unsigned int)>&& on_key_accepted);

private:
    struct Self;
    explicit SlowKeys(std::shared_ptr<Self>);
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_SLOW_KEYS_H
