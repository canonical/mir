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

#ifndef MIRAL_WINDOW_H
#define MIRAL_WINDOW_H

#include "miral/application.h"

#include <mir/geometry/point.h>
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

    /// The position of the top-left corner of the window frame
    auto top_left()     const -> mir::geometry::Point;
    /// The size of the window frame. Units are logical screen coordinates (not necessarily device pixels). Any
    /// decorations are included in the size.
    auto size()         const -> mir::geometry::Size;
    /// The application that created this window
    auto application()  const -> Application;

    /// Indicates that the Window isn't null
    operator bool() const;

    /// Not for external use, use WindowManagerTools::modify_window() instead
    /// @{
    void resize(mir::geometry::Size const& size);
    void move_to(mir::geometry::Point top_left);
    /// @}

    /// Access to the underlying Mir surface
    /// @{
    operator std::weak_ptr<mir::scene::Surface>() const;
    operator std::shared_ptr<mir::scene::Surface>() const;
    /// @}

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

/// Customization for Google test (to print surface name in errors)
/// \see https://github.com/google/googletest/blob/main/docs/advanced.md#teaching-googletest-how-to-print-your-values
void PrintTo(Window const& bar, std::ostream* os);
}

#endif //MIRAL_WINDOW_H
