/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_WINDOW_H
#define MIRAL_WINDOW_H

#include "miral/application.h"

#include <mir/optional_value.h>
#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/size.h>

#include <memory>

namespace mir
{
namespace scene { class Surface; }
}

namespace miral
{
/// Handle class to manage a Mir surface. It may be null (e.g. default initialized)
class Window
{
public:
    Window();
    Window(Application const& application, std::shared_ptr<mir::scene::Surface> const& surface);
    ~Window();

    auto top_left()     const -> mir::geometry::Point;
    auto size()         const -> mir::geometry::Size;
    auto application()  const -> Application;

    // Indicates that the Window isn't null
    operator bool() const;

    void resize(mir::geometry::Size const& size);

    void move_to(mir::geometry::Point top_left);

    auto exclusive_display() const -> mir::optional_value<mir::geometry::Rectangle>;
    void set_exclusive_display(mir::optional_value<mir::geometry::Rectangle> display);

    // Access to the underlying Mir surface
    operator std::weak_ptr<mir::scene::Surface>() const;
    operator std::shared_ptr<mir::scene::Surface>() const;

private:
    struct Self;
    std::shared_ptr <Self> self;

    friend bool operator==(Window const& lhs, Window const& rhs);
    friend bool operator==(std::shared_ptr<mir::scene::Surface> const& lhs, Window const& rhs);
    friend bool operator==(Window const& lhs, std::shared_ptr<mir::scene::Surface> const& rhs);
    friend bool operator<(Window const& lhs, Window const& rhs);
};

bool operator==(Window const& lhs, Window const& rhs);
bool operator==(std::shared_ptr<mir::scene::Surface> const& lhs, Window const& rhs);
bool operator==(Window const& lhs, std::shared_ptr<mir::scene::Surface> const& rhs);
bool operator<(Window const& lhs, Window const& rhs);

inline bool operator!=(Window const& lhs, Window const& rhs) { return !(lhs == rhs); }
inline bool operator!=(std::shared_ptr<mir::scene::Surface> const& lhs, Window const& rhs) { return !(lhs == rhs); }
inline bool operator!=(Window const& lhs, std::shared_ptr<mir::scene::Surface> const& rhs) { return !(lhs == rhs); }
inline bool operator>(Window const& lhs, Window const& rhs) { return rhs < lhs; }
inline bool operator<=(Window const& lhs, Window const& rhs) { return !(lhs > rhs); }
inline bool operator>=(Window const& lhs, Window const& rhs) { return !(lhs < rhs); }
}

#endif //MIRAL_WINDOW_H
