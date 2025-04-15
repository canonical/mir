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

#ifndef MIRAL_LOCATE_POINTER_H
#define MIRAL_LOCATE_POINTER_H

#include <miral/toolkit_event.h>

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
class LocatePointer
{
public:
    LocatePointer(bool enabled_by_default);

    void operator()(mir::Server& server);

    LocatePointer& delay(std::chrono::milliseconds delay);
    LocatePointer& on_locate_pointer(std::function<void(float x, float y)>&&);
    LocatePointer& on_enabled(std::function<void()>&&);
    LocatePointer& on_disabled(std::function<void()>&&);
    LocatePointer& key_combination(std::function<bool(MirInputEvent const*)>);

    LocatePointer& enable();
    LocatePointer& disable();

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_LOCATE_POINTER_H


