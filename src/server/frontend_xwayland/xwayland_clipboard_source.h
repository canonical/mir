/*
 * Copyright (C) 2021 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_XWAYLAND_CLIPBOARD_SOURCE_H_
#define MIR_FRONTEND_XWAYLAND_CLIPBOARD_SOURCE_H_

#include "xcb_connection.h"

struct xcb_xfixes_selection_notify_event_t;

namespace mir
{
namespace scene
{
class Clipboard;
class ClipboardSource;
}
namespace frontend
{
/// Exposes X11 selections to non-X11 clients
class XWaylandClipboardSource
{
public:
    XWaylandClipboardSource(
        XCBConnection& connection,
        std::shared_ptr<scene::Clipboard> const& clipboard);
    ~XWaylandClipboardSource();

    /// Return's if the given clipboard source is an XWayland source for the given connection. This is used to prevent
    /// stealing the X11 clipboard when the source is from X11. It must check if the connection is the same because
    /// if we're running multiple XWaylands sources coming from different ones should be treated like any other.
    static auto source_is_from(scene::ClipboardSource* source, XCBConnection& connection) -> bool;

    /// Called by the source, indicates the XWayland buffer needs to be sent to the given fd
    void initiate_send(xcb_atom_t target_type, Fd const& receiver_fd);

    /// Called by the wm
    /// @{
    void selection_notify_event(xcb_selection_notify_event_t* event);
    void xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event);
    /// @}

private:
    class ClipboardSource;

    XWaylandClipboardSource(XWaylandClipboardSource const&) = delete;
    XWaylandClipboardSource& operator=(XWaylandClipboardSource const&) = delete;

    /// Creates a new source and sends it to the clipboard. Aborts if timestamp isn't equal to
    /// clipboard_ownership_timestamp when the source is ready.
    void create_source(xcb_timestamp_t timestamp, std::vector<xcb_atom_t> const& targets);

    /// Sends the given data to the current destination fd and then closes and removes the fd
    void send_data_to_fd(uint8_t const* data, size_t size);

    XCBConnection& connection;
    std::shared_ptr<scene::Clipboard> const clipboard;
    xcb_window_t const receiving_window;

    std::mutex mutex;
    xcb_window_t current_clipbaord_owner{XCB_WINDOW_NONE};
    xcb_timestamp_t clipboard_ownership_timestamp{0};
    std::shared_ptr<ClipboardSource> clipboard_source;
    std::optional<Fd> pending_send_fd;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIPBOARD_SOURCE_H_
