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
namespace dispatch
{
class MultiplexingDispatchable;
}
namespace frontend
{
/// Exposes X11 selections to non-X11 clients
class XWaylandClipboardSource
{
public:
    XWaylandClipboardSource(
        XCBConnection& connection,
        std::shared_ptr<dispatch::MultiplexingDispatchable> const& dispatcher,
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
    void property_notify_event(xcb_window_t window, xcb_atom_t property);
    /// @}

private:
    class ClipboardSource;
    class DataSender;

    XWaylandClipboardSource(XWaylandClipboardSource const&) = delete;
    XWaylandClipboardSource& operator=(XWaylandClipboardSource const&) = delete;

    /// Creates a new source and sends it to the clipboard. Aborts if timestamp isn't equal to
    /// clipboard_ownership_timestamp when the source is ready.
    void create_source(xcb_timestamp_t timestamp, std::vector<xcb_atom_t> const& targets);

    /// Called when there is new data in the _WL_SELECTION property that needs to be sent to the in-progress send
    void read_and_send_wl_selection_data(std::lock_guard<std::mutex> const& lock);

    /// Sends the given data to the current destination fd
    void add_data_to_in_progress_send(std::lock_guard<std::mutex> const& lock, uint8_t* data_ptr, size_t data_size);

    XCBConnection& connection;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const dispatcher;
    std::shared_ptr<scene::Clipboard> const clipboard;
    xcb_window_t const receiving_window;

    std::mutex mutex;
    xcb_window_t current_clipbaord_owner{XCB_WINDOW_NONE};
    xcb_timestamp_t clipboard_ownership_timestamp{0};
    std::shared_ptr<ClipboardSource> clipboard_source;
    bool incremental_transfer_in_progress{false};
    std::shared_ptr<DataSender> in_progress_send;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIPBOARD_SOURCE_H_
