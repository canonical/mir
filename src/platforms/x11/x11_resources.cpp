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

namespace
{
class BasicXCBConnection : public mx::XCBConnection
{
public:
    BasicXCBConnection(xcb_connection_t* conn)
        : conn{conn},
          screen_{xcb_setup_roots_iterator(xcb_get_setup(conn)).data}
    {
    }

    // xcb_disconnect() is handled elsewhere by XCloseDisplay()

    auto has_error() const -> int override
    {
        return xcb_connection_has_error(conn);
    }

    auto get_file_descriptor() const -> int override
    {
        return xcb_get_file_descriptor(conn);
    }

    auto poll_for_event() const -> xcb_generic_event_t* override
    {
        return xcb_poll_for_event(conn);
    }

    auto screen() const -> xcb_screen_t* override
    {
        return screen_;
    }

    auto intern_atom(std::string const& name) const -> xcb_atom_t override
    {
        auto const cookie = xcb_intern_atom(conn, 0, name.size(), name.c_str());
        auto const reply = xcb_intern_atom_reply(conn, cookie, nullptr);
        auto const atom = reply->atom;
        free(reply);
        return atom;
    }

    auto generate_id() const -> uint32_t override
    {
        return xcb_generate_id(conn);
    }

    void create_window(
        xcb_window_t win,
        int16_t x, int16_t y,
        uint32_t value_mask,
        const void* value_list) const override
    {
        xcb_create_window(
            conn,
            XCB_COPY_FROM_PARENT,
            win,
            screen_->root,
            0, 0,
            x, y,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen_->root_visual,
            value_mask,
            value_list);
    }

    void change_property(
        xcb_window_t window,
        xcb_atom_t property_atom,
        xcb_atom_t type_atom,
        uint8_t format,
        size_t length,
        void const* data) const override
    {
        xcb_change_property(
            conn,
            XCB_PROP_MODE_REPLACE,
            window,
            property_atom,
            type_atom,
            format,
            length,
            data);
    }

    void map_window(xcb_window_t window) const override
    {
        xcb_map_window(conn, window);
    }

    void destroy_window(xcb_window_t window) const override
    {
        xcb_map_window(conn, window);
    }

    void flush() const override
    {
        xcb_flush(conn);
    }

    auto connection() const -> xcb_connection_t* override
    {
        return conn;
    }

private:
    xcb_connection_t* const conn; ///< Used for most things
    xcb_screen_t* const screen_;
};
}

std::mutex mx::X11Resources::instance_mutex;
std::weak_ptr<mx::X11Resources> mx::X11Resources::instance_;

auto mx::X11Resources::instance() -> std::shared_ptr<X11Resources>
{
    std::lock_guard<std::mutex> lock{instance_mutex};

    if (auto resources = instance_.lock())
    {
        return resources;
    }

    XInitThreads();

    auto const xlib_dpy = XOpenDisplay(nullptr);
    if (!xlib_dpy)
    {
        // Faled to open X11 display, probably X isn't running
        return nullptr;
    }
    XSetEventQueueOwner(xlib_dpy, XCBOwnsEventQueue);

    auto const xcb_conn = XGetXCBConnection(xlib_dpy);
    if (!xcb_conn || xcb_connection_has_error(xcb_conn))
    {
        log_error("XGetXCBConnection() failed");
        XCloseDisplay(xlib_dpy); // closes XCB connection if needed
        return nullptr;
    }

    auto const resources = std::make_shared<X11Resources>(std::make_unique<BasicXCBConnection>(xcb_conn), xlib_dpy);
    instance_ = resources;
    return resources;
}

mx::X11Resources::X11Resources(std::unique_ptr<XCBConnection>&& conn, ::Display* xlib_dpy)
    : conn{std::move(conn)},
      xlib_dpy{xlib_dpy},
      UTF8_STRING{this->conn->intern_atom("UTF8_STRING")},
      _NET_WM_NAME{this->conn->intern_atom("_NET_WM_NAME")},
      WM_PROTOCOLS{this->conn->intern_atom("WM_PROTOCOLS")},
      WM_DELETE_WINDOW{this->conn->intern_atom("WM_DELETE_WINDOW")}
{
}

mx::X11Resources::~X11Resources()
{
    if (xlib_dpy)
    {
        XCloseDisplay(xlib_dpy); // calls xcb_disconnect() for us
    }
}

void mx::X11Resources::set_set_output_for_window(xcb_window_t win, VirtualOutput* output)
{
    std::lock_guard<std::mutex> lock{outputs_mutex};
    outputs[win] = output;
}

void mx::X11Resources::clear_output_for_window(xcb_window_t win)
{
    std::lock_guard<std::mutex> lock{outputs_mutex};
    outputs.erase(win);
}

void mx::X11Resources::with_output_for_window(
    xcb_window_t win,
    std::function<void(std::optional<VirtualOutput*> output)> fn)
{
    std::lock_guard<std::mutex> lock{outputs_mutex};
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
