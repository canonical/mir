/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MIR_FRONTEND_XWAYLAND_CURSORS_H
#define MIR_FRONTEND_XWAYLAND_CURSORS_H

#include <memory>
#include <vector>
#include <experimental/optional>
#include <X11/Xcursor/Xcursor.h>
#include <xcb/composite.h>
#include <xcb/xcb.h>

namespace mir
{
namespace frontend
{
class XCBConnection;

class XWaylandCursors
{
public:
    XWaylandCursors(std::shared_ptr<XCBConnection> const& connection);
    void apply_default_to(xcb_window_t window) const;

private:
    struct Cursor
    {
        Cursor(std::shared_ptr<XCBConnection> const& connection, xcb_cursor_t xcb_cursor);
        ~Cursor();

        Cursor(Cursor const&) = delete;
        auto operator=(Cursor const&) -> bool = delete;

        void apply_to(xcb_window_t window) const;

        std::shared_ptr<XCBConnection> const connection;
        xcb_cursor_t const xcb_cursor;
    };

    struct Loader
    {
        struct Formats {
            // std::experimental::optional<xcb_render_pictforminfo_t> rgb;
            std::experimental::optional<xcb_render_pictforminfo_t> rgba;
        };

        Loader(std::shared_ptr<XCBConnection> const& connection);
        static auto query_formats(std::shared_ptr<XCBConnection> const& connection) -> Loader::Formats;
        static auto get_xcursor_size() -> int;

        /// Can return null
        auto load_cursor(std::string const& name) const -> std::unique_ptr<Cursor>;
        /// Can return null
        auto load_default() const -> std::unique_ptr<Cursor>;

        std::shared_ptr<XCBConnection> const connection;
        Formats const formats;
        int const cursor_size;
    };

    Loader const loader;
    std::unique_ptr<Cursor> const default_cursor; ///< Can be null
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CURSORS_H
