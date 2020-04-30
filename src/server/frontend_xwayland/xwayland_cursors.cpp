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
 * Written with alot of help of Weston Xwm
 *
 */

#include "xcb_connection.h"
#include "xwayland_cursors.h"
#include "xwayland_log.h"

#include <cstring>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;

// Cursor names that may be useful in the future:
// {"bottom_left_corner", "sw-resize", "size_bdiag"};
// {"bottom_right_corner", "se-resize", "size_fdiag"};
// {"bottom_side", "s-resize", "size_ver"};
// {"left_ptr", "default", "top_left_arrow", "left-arrow"};
// {"left_side", "w-resize", "size_hor"};
// {"right_side", "e-resize", "size_hor"};
// {"top_left_corner", "nw-resize", "size_fdiag"};
// {"top_right_corner", "ne-resize", "size_bdiag"};
// {"top_side", "n-resize", "size_ver"};

namespace
{
std::initializer_list<std::string> const default_cursor_names{
    "left_ptr", "default", "top_left_arrow", "left-arrow"};
}

mf::XWaylandCursors::XWaylandCursors(std::shared_ptr<XCBConnection> const& connection)
    : loader{connection},
      default_cursor{loader.load_default()}
{
}

void mf::XWaylandCursors::apply_default_to(xcb_window_t window) const
{
    if (default_cursor)
    {
        default_cursor->apply_to(window);
    }
    else
    {
        log_error("No default cursor loaded");
    }
}

mf::XWaylandCursors::Cursor::Cursor(std::shared_ptr<XCBConnection> const& connection, xcb_cursor_t xcb_cursor)
    : connection{connection},
      xcb_cursor{xcb_cursor}
{
}

mf::XWaylandCursors::Cursor::~Cursor()
{
    xcb_free_cursor(*connection, xcb_cursor);
}

void mf::XWaylandCursors::Cursor::apply_to(xcb_window_t window) const
{
    xcb_change_window_attributes(*connection, window, XCB_CW_CURSOR, &xcb_cursor);
    connection->flush();
}

mf::XWaylandCursors::Loader::Loader(std::shared_ptr<XCBConnection> const& connection)
    : connection{connection},
      formats{query_formats(connection)},
      cursor_size{get_xcursor_size()}
{
}

auto mf::XWaylandCursors::Loader::query_formats(std::shared_ptr<XCBConnection> const& connection) -> Loader::Formats
{
    auto const formats_cookie = xcb_render_query_pict_formats(*connection);
    auto const formats_reply = xcb_render_query_pict_formats_reply(*connection, formats_cookie, 0);
    mf::XWaylandCursors::Loader::Formats result;
    if (formats_reply)
    {
        auto const formats = xcb_render_query_pict_formats_formats(formats_reply);
        for (unsigned i = 0; i < formats_reply->num_formats; i++)
        {
            if (formats[i].direct.red_mask != 0xff && formats[i].direct.red_shift != 16)
            {
                continue;
            }

            // if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 24)
            // {
            //    result.rgb = formats[i];
            // }

            if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 32 &&
                formats[i].direct.alpha_mask == 0xff && formats[i].direct.alpha_shift == 24)
            {
                result.rgba = formats[i];
            }
        }

        free(formats_reply);
    }
    else
    {
        log_warning("Could not get color formats from the X server");
    }
    return result;
}

auto mf::XWaylandCursors::Loader::get_xcursor_size() -> int
{
    char const* size_env_var_string = getenv("XCURSOR_SIZE");
    int result = 0;
    if (size_env_var_string)
    {
        result = atoi(size_env_var_string);
    }
    if (result == 0)
    {
        result = 32;
    }
    return result;
}

auto mf::XWaylandCursors::Loader::load_cursor(std::string const& name) const -> std::unique_ptr<Cursor>
{
    auto const images = XcursorLibraryLoadImages(name.c_str(), nullptr, cursor_size);

    if (!images || images->nimage != 1)
    {
        return nullptr;
    }
    auto const img = images->images[0];

    if (!formats.rgba)
    {
        return nullptr;
    }
    auto const format = formats.rgba.value();

    xcb_pixmap_t const pix = xcb_generate_id(*connection);
    xcb_create_pixmap(*connection, 32, pix, connection->screen()->root, img->width, img->height);

    xcb_render_picture_t const pic = xcb_generate_id(*connection);
    xcb_render_create_picture(*connection, pic, pix, format.id, 0, 0);

    xcb_gcontext_t const gc = xcb_generate_id(*connection);
    xcb_create_gc(*connection, gc, pix, 0, 0);

    int const stride = img->width * 4;
    xcb_put_image(*connection, XCB_IMAGE_FORMAT_Z_PIXMAP, pix, gc, img->width, img->height, 0, 0, 0, 32, stride * img->height,
                  (uint8_t *)img->pixels);
    xcb_free_gc(*connection, gc);

    xcb_cursor_t const cursor = xcb_generate_id(*connection);
    xcb_render_create_cursor(*connection, cursor, pic, img->xhot, img->yhot);

    xcb_render_free_picture(*connection, pic);
    xcb_free_pixmap(*connection, pix);

    XcursorImagesDestroy(images);

    return std::make_unique<Cursor>(connection, cursor);
}

auto mf::XWaylandCursors::Loader::load_default() const -> std::unique_ptr<Cursor>
{
    for (auto const& name : default_cursor_names)
    {
        if (auto cursor = load_cursor(name))
        {
            return cursor;
        }
    }
    log_warning("Failed to load any default cursor");
    return nullptr;
}
