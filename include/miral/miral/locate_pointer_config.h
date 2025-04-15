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

#ifndef MIRAL_LOCATE_POINTER_CONFIG_H
#define MIRAL_LOCATE_POINTER_CONFIG_H

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
class LocatePointerConfig
{
public:
    LocatePointerConfig(bool enabled_by_default);

    void operator()(mir::Server& server) const;

    void delay(std::chrono::milliseconds delay) const;
    void on_locate_pointer(std::function<void(float x, float y)>&&) const;
    void enabled(bool enabled) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_LOCATE_POINTER_CONFIG_H


