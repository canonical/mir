/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#define MIR_LOG_COMPONENT "x11-error"
#include "mir/log.h"

#include "x11_resources.h"

#include <X11/Xlib-xcb.h>

namespace mg=mir::graphics;
namespace mx = mir::X;

mx::X11Resources mx::X11Resources::instance;

mx::X11Connection::X11Connection(xcb_connection_t* conn, ::Display* xlib_dpy)
    : conn{conn},
      xlib_dpy{xlib_dpy}
{
}

mx::X11Connection::~X11Connection()
{
    XCloseDisplay(xlib_dpy); // calls xcb_disconnect() for us
}

auto mx::X11Resources::get_conn() -> std::shared_ptr<X11Connection>
{
    std::lock_guard<std::mutex> lock{mutex};

    if (auto conn = connection.lock())
        return conn;

    XInitThreads();

    auto const xlib_dpy = XOpenDisplay(nullptr);
    if (!xlib_dpy)
    {
        // Faled to open X11 display, probably X isn't running
        return nullptr;
    }

    auto const xcb_conn = XGetXCBConnection(xlib_dpy);
    if (!xcb_conn || xcb_connection_has_error(xcb_conn))
    {
        log_error("XGetXCBConnection() failed");
        XCloseDisplay(xlib_dpy); // closes XCB connection if needed
        return nullptr;
    }

    auto const new_conn = std::make_shared<X11Connection>(xcb_conn, xlib_dpy);
    connection = new_conn;
    return new_conn;
}

void mx::X11Resources::set_set_output_for_window(xcb_window_t win, VirtualOutput* output)
{
    std::lock_guard<std::mutex> lock{mutex};
    outputs[win] = output;
}

void mx::X11Resources::clear_output_for_window(xcb_window_t win)
{
    std::lock_guard<std::mutex> lock{mutex};
    outputs.erase(win);
}

void mx::X11Resources::with_output_for_window(
    xcb_window_t win,
    std::function<void(std::optional<VirtualOutput*> output)> fn)
{
    std::lock_guard<std::mutex> lock{mutex};
    auto const iter = outputs.find(win);
    if (iter != outputs.end())
    {
        return fn(iter->second);
    }
    else
    {
        return fn(std::nullopt);
    }
}
