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

#ifndef MIRAL_DECORATION_H
#define MIRAL_DECORATION_H


#include "mir/geometry/forward.h"
#include <memory>
#include <functional>

namespace mir
{
namespace shell
{
class Shell;
namespace decoration
{
struct DeviceEvent;
}
}
namespace scene
{
class Surface;
}
}

namespace miral
{
class DecorationAdapter;

// Maybe it'd be best to move Device event to somewhere common between mir and miral?
class DeviceEvent
{
public:
    DeviceEvent(mir::shell::decoration::DeviceEvent);
    operator mir::shell::decoration::DeviceEvent() const;


    auto location() const -> mir::geometry::Point;
    auto pressed() const -> bool;

private:
    struct Impl;
    std::shared_ptr<Impl> impl;
};

struct DecorationRedrawNotifier
{
    using OnRedraw = std::function<void()>;
    void notify()
    {
        if (on_redraw)
            on_redraw();
    }

    void register_listener(OnRedraw on_redraw)
    {
        this->on_redraw = on_redraw;
    }

    OnRedraw on_redraw;
};

using Pixel = uint32_t;

class Decoration // Placeholder names
{
public:
    using DecorationBuilder = std::function<std::shared_ptr<DecorationAdapter>()>;

    Decoration();

    std::shared_ptr<miral::DecorationRedrawNotifier> redraw_notifier();

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif
