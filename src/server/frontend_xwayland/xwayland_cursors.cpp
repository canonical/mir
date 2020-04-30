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

namespace
{
template<typename T, size_t length>
constexpr size_t length_of(T(&)[length])
{
    return length;
}
}

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) length_of(a)
#endif

#define CURSOR_ENTRY(x)        \
    {                          \
        (x), ARRAY_LENGTH((x)) \
    }

static const char *bottom_left_corners[] = {"bottom_left_corner", "sw-resize", "size_bdiag"};

static const char *bottom_right_corners[] = {"bottom_right_corner", "se-resize", "size_fdiag"};

static const char *bottom_sides[] = {"bottom_side", "s-resize", "size_ver"};

static const char *left_ptrs[] = {"left_ptr", "default", "top_left_arrow", "left-arrow"};

static const char *left_sides[] = {"left_side", "w-resize", "size_hor"};

static const char *right_sides[] = {"right_side", "e-resize", "size_hor"};

static const char *top_left_corners[] = {"top_left_corner", "nw-resize", "size_fdiag"};

static const char *top_right_corners[] = {"top_right_corner", "ne-resize", "size_bdiag"};

static const char *top_sides[] = {"top_side", "n-resize", "size_ver"};

struct cursor_alternatives
{
    const char **names;
    size_t count;
};

static const struct cursor_alternatives cursors[] = {
    CURSOR_ENTRY(top_sides),           CURSOR_ENTRY(bottom_sides),         CURSOR_ENTRY(left_sides),
    CURSOR_ENTRY(right_sides),         CURSOR_ENTRY(top_left_corners),     CURSOR_ENTRY(top_right_corners),
    CURSOR_ENTRY(bottom_left_corners), CURSOR_ENTRY(bottom_right_corners), CURSOR_ENTRY(left_ptrs)};

namespace mf = mir::frontend;

mf::XWaylandCursors::XWaylandCursors(std::shared_ptr<XCBConnection> const& connection)
    : connection{connection}
{
    auto const formats_cookie = xcb_render_query_pict_formats(*connection);
    auto const formats_reply = xcb_render_query_pict_formats_reply(*connection, formats_cookie, 0);
    if (formats_reply)
    {
        auto const formats = xcb_render_query_pict_formats_formats(formats_reply);
        for (unsigned i = 0; i < formats_reply->num_formats; i++)
        {
            if (formats[i].direct.red_mask != 0xff && formats[i].direct.red_shift != 16)
                continue;
            if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 24)
                xcb_format_rgb = formats[i];
            if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT && formats[i].depth == 32 &&
                formats[i].direct.alpha_mask == 0xff && formats[i].direct.alpha_shift == 24)
                xcb_format_rgba = formats[i];
        }

        free(formats_reply);
    }
    else
    {
        log_warning("Could not get color formats from the X server");
    }

    create_wm_cursor();
    set_cursor(connection->root_window(), CursorLeftPointer);
}

mf::XWaylandCursors::~XWaylandCursors()
{
    for (auto xcb_cursor : xcb_cursors)
    {
        xcb_free_cursor(*connection, xcb_cursor);
    }
}

void mf::XWaylandCursors::set_cursor(xcb_window_t id, const CursorType &cursor)
{
    if (xcb_cursor == cursor)
        return;

    xcb_cursor = cursor;
    uint32_t cursor_value_list = xcb_cursors[cursor];
    xcb_change_window_attributes(*connection, id, XCB_CW_CURSOR, &cursor_value_list);
    connection->flush();
}

void mf::XWaylandCursors::create_wm_cursor()
{
    const char *name;
    int count = ARRAY_LENGTH(cursors);

    xcb_cursors.clear();
    xcb_cursors.reserve(count);

    for (int i = 0; i < count; i++)
    {
        for (size_t j = 0; j < cursors[i].count; j++)
        {
            name = cursors[i].names[j];
            xcb_cursors.push_back(xcb_cursor_library_load_cursor(name));
            if (xcb_cursors[i] != static_cast<xcb_cursor_t>(-1))
                break;
        }
    }
}

// Cursor
xcb_cursor_t mf::XWaylandCursors::xcb_cursor_image_load_cursor(const XcursorImage *img)
{
    xcb_connection_t *c = *connection;
    xcb_screen_iterator_t s = xcb_setup_roots_iterator(xcb_get_setup(c));
    xcb_screen_t *screen = s.data;
    xcb_gcontext_t gc;
    xcb_pixmap_t pix;
    xcb_render_picture_t pic;
    xcb_cursor_t cursor;
    int stride = img->width * 4;

    pix = xcb_generate_id(c);
    xcb_create_pixmap(c, 32, pix, screen->root, img->width, img->height);

    pic = xcb_generate_id(c);
    xcb_render_create_picture(c, pic, pix, xcb_format_rgba.id, 0, 0);

    gc = xcb_generate_id(c);
    xcb_create_gc(c, gc, pix, 0, 0);

    xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, pix, gc, img->width, img->height, 0, 0, 0, 32, stride * img->height,
                  (uint8_t *)img->pixels);
    xcb_free_gc(c, gc);

    cursor = xcb_generate_id(c);
    xcb_render_create_cursor(c, cursor, pic, img->xhot, img->yhot);

    xcb_render_free_picture(c, pic);
    xcb_free_pixmap(c, pix);

    return cursor;
}

xcb_cursor_t mf::XWaylandCursors::xcb_cursor_images_load_cursor(const XcursorImages *images)
{
    if (images->nimage != 1)
        return -1;

    return xcb_cursor_image_load_cursor(images->images[0]);
}

xcb_cursor_t mf::XWaylandCursors::xcb_cursor_library_load_cursor(const char *file)
{
    xcb_cursor_t cursor;
    XcursorImages *images;
    char *v = NULL;
    int size = 0;

    if (!file)
        return 0;

    v = getenv("XCURSOR_SIZE");
    if (v)
        size = atoi(v);

    if (!size)
        size = 32;

    images = XcursorLibraryLoadImages(file, NULL, size);
    if (!images)
        return -1;

    cursor = xcb_cursor_images_load_cursor(images);
    XcursorImagesDestroy(images);

    return cursor;
}
