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
    ~XWaylandCursors();

private:
    enum CursorType
    {
        CursorUnset = -1,
        CursorTop,
        CursorBottom,
        CursorLeft,
        CursorRight,
        CursorTopLeft,
        CursorTopRight,
        CursorBottomLeft,
        CursorBottomRight,
        CursorLeftPointer
    };

    void set_cursor(xcb_window_t id, const CursorType &cursor);
    void create_wm_cursor();

    // Cursor
    xcb_cursor_t xcb_cursor_image_load_cursor(const XcursorImage *img);
    xcb_cursor_t xcb_cursor_images_load_cursor(const XcursorImages *images);
    xcb_cursor_t xcb_cursor_library_load_cursor(const char *file);

    std::shared_ptr<XCBConnection> const connection;
    xcb_render_pictforminfo_t xcb_format_rgb, xcb_format_rgba;
    std::vector<xcb_cursor_t> xcb_cursors;
    int xcb_cursor;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CURSORS_H
