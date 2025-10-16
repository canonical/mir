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

#include <miral/application.h>

#include <mir/geometry/point.h>
#include <mir/geometry/size.h>

#include <memory>

namespace mir
{
namespace scene { class Surface; }
}

namespace miral
{
/// A class providing access to a Wayland or X11 surface.
///
/// Users are not expected to instantiate instances of this class themselves.
/// Instead, they may implement #miral::WindowManagementPolicy::advise_new_window to
/// be notified when the server has received a new window. Similarly, they may
/// implement #miral::WindowManagementPolicy::advise_delete_window to be notified
/// when a window has been destroyed.
///
/// If the window is default constructed, it will not have any surface backing it.
///
/// \sa miral::WindowManagementPolicy::advise_new_window - gets called when a new
///     window has been created by the server
/// \sa miral::WindowInfo - provides additional information for a #miral::Window instance
class Window
{
public:
    /// Construct an empty window instance.
    ///
    /// This window is not backed by anything, meaning that all methods on it will
    /// return stub values.
    Window();

    /// Construct a window backed by \p surface for \p application.
    ///
    /// \param application the application to which this window belongs
    /// \param surface the raw surface backing this window
    Window(Application const& application, std::shared_ptr<mir::scene::Surface> const& surface);
    ~Window();

    /// Retrieve the position of the top-left corner of the window frame
    ///
    /// \returns the top-left point of the frame of the window
    auto top_left()     const -> mir::geometry::Point;

    /// Retrieve the size of the window frame.
    ///
    /// Units are logical screen coordinates (not necessarily device pixels). Any
    /// decorations are included in the size.
    ///
    /// \returns the size of the window, including decorations
    auto size()         const -> mir::geometry::Size;

    /// Retrieve the application that created this window.
    ///
    /// \returns the application
    auto application()  const -> Application;

    /// Checks whether the backing surface is valid or not.
    ///
    /// This will return true if the window was default constructed, or if the surface
    /// backing it is now invalid.
    operator bool() const;

    /// Resize the window to the given \p size.
    ///
    /// \param size the new size
    /// \note Not for external use, use #WindowManagerTools::modify_window instead.
    void resize(mir::geometry::Size const& size);

    /// Move the window to the point given by \p top_left.
    ///
    /// \param top_left point to move to
    /// \note Not for external use, use #WindowManagerTools::modify_window instead.
    void move_to(mir::geometry::Point top_left);

    /// Access the surface backing this window as a weak pointer.
    ///
    /// \returns weak pointer to the scene
    operator std::weak_ptr<mir::scene::Surface>() const;

    /// Access the surface backing this window as a shared pointer.
    ///
    /// \returns shared pointer to the scene
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

/// Customization for Google test (to print surface name in errors)
/// \see https://github.com/google/googletest/blob/main/docs/advanced.md#teaching-googletest-how-to-print-your-values
void PrintTo(Window const& bar, std::ostream* os);
}

#endif //MIRAL_WINDOW_H
