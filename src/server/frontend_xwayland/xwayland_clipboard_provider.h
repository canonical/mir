/*
 * Copyright (C) Canonical Ltd.
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

#ifndef MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_
#define MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_

#include "xcb_connection.h"
#include <mir/scene/data_exchange.h>

#include <map>

struct xcb_xfixes_selection_notify_event_t;

namespace mir
{
namespace scene
{
class Clipboard;
class DataExchangeSource;
}
namespace dispatch
{
class MultiplexingDispatchable;
}
namespace frontend
{
/// Exposes non-X11 selections to X11 clients
class XWaylandClipboardProvider
{
public:
    XWaylandClipboardProvider(
        std::shared_ptr<XCBConnection> const& connection,
        std::shared_ptr<dispatch::MultiplexingDispatchable> const& dispatcher,
        std::shared_ptr<scene::Clipboard> const& clipboard);
    ~XWaylandClipboardProvider();

    /// Called by the wm
    /// @{
    void selection_request_event(xcb_selection_request_event_t* event);
    void xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event);
    void property_deleted_event(xcb_window_t window, xcb_atom_t property);
    /// @}

private:
    class SelectionSender;
    class ClipboardObserver;

    struct ThreadsafeSelf
    {
        ThreadsafeSelf(XWaylandClipboardProvider* ptr)
            : ptr{ptr}
        {
        }

        std::mutex mutex;
        XWaylandClipboardProvider* ptr; ///< nulled out when provider is destroyed
    };

    XWaylandClipboardProvider(XWaylandClipboardProvider const&) = delete;
    XWaylandClipboardProvider& operator=(XWaylandClipboardProvider const&) = delete;

    /// Sends the list of possible targets to a client
    void send_targets(
        xcb_timestamp_t time,
        xcb_window_t requester,
        xcb_atom_t property);

    /// Sends the timestamp that we took ownership of the clipboard
    void send_timestamp(
        xcb_timestamp_t time,
        xcb_window_t requester,
        xcb_atom_t property);

    /// Send the actual data from the current data source
    void send_data(
        xcb_timestamp_t time,
        xcb_window_t requester,
        xcb_atom_t property,
        xcb_atom_t target,
        std::string const& mime_type);

    /// Called by the observer, indicates the paste source has been set by someone (could be us or Wayland)
    void paste_source_set(std::shared_ptr<scene::DataExchangeSource> const& source);

    /// Called by the sender, indicates sending should resume when the sender's property is deleted
    void defer_incremental_send(std::shared_ptr<SelectionSender> sender, xcb_window_t window, xcb_atom_t property);

    std::shared_ptr<ThreadsafeSelf> const threadsafe_self;
    std::shared_ptr<XCBConnection> const connection;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const dispatcher;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<ClipboardObserver> const clipboard_observer;
    xcb_window_t const selection_window;

    std::mutex mutex;
    /// The timestamp of when we took ownership of the clipboard. May be XCB_TIME_CURRENT_TIME or outdated if we haven't
    /// received the event we need to set it.
    xcb_timestamp_t clipboard_ownership_timestamp{XCB_TIME_CURRENT_TIME};
    /// The source X11 clients can paste from. Should never be a source from this XWayland connection. Can be null
    std::shared_ptr<scene::DataExchangeSource> current_source;
    /// Maps window/property pairs to SelectionSender's that are waiting for those properties to be deleted in order to
    /// continue an incremental send.
    std::map<std::pair<xcb_window_t, xcb_atom_t>, std::shared_ptr<SelectionSender>> pending_incremental_sends;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIPBOARD_PROVIDER_H_
