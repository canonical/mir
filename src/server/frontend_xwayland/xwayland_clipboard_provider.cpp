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

#include "xwayland_clipboard_provider.h"
#include "xwayland_clipboard_source.h"

#include "xwayland_log.h"
#include "mir/scene/clipboard.h"

#include <xcb/xfixes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;

namespace
{
size_t const INCREMENT_CHUNK_SIZE = 64 * 1024;

auto create_selection_window(mf::XCBConnection const& connection) -> xcb_window_t
{
    uint32_t const attrib_values[]{XCB_EVENT_MASK_PROPERTY_CHANGE};

    xcb_window_t const selection_window = xcb_generate_id(connection);
    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        selection_window,
        connection.root_window(),
        0, 0, 10, 10, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection.screen()->root_visual,
        XCB_CW_EVENT_MASK, attrib_values);

    return selection_window;
}

class ReadError : public std::runtime_error
{
public:
    ReadError() : runtime_error{strerror(errno)}
    {
    }
};

/// Like read(), but calls read() until the end of the file or the buffer fills up
auto read_repeatedly(mir::Fd const& fd, uint8_t* buffer, size_t size) -> size_t
{
    size_t total_len = 0;
    while (true)
    {
        auto const remaining = size - total_len;
        if (remaining <= 0)
        {
            break; // buffer full
        }
        auto const len = read(fd, buffer + total_len, remaining);
        if (len < 0)
        {
            throw ReadError();
        }
        total_len += len;
        if (len == 0)
        {
            break; // end of file
        }
    }
    return total_len;
}
}

class mf::XWaylandClipboardProvider::ClipboardObserver : public scene::ClipboardObserver
{
public:
    ClipboardObserver(XWaylandClipboardProvider* const owner)
        : owner{owner}
    {
    }

    void paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source) override
    {
        owner->paste_source_set(source);
    }

private:
    XWaylandClipboardProvider* const owner;
};

mf::XWaylandClipboardProvider::XWaylandClipboardProvider(
    XCBConnection& connection,
    std::shared_ptr<scene::Clipboard> const& clipboard)
    : connection{connection},
      clipboard{clipboard},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)},
      selection_window{create_selection_window(connection)}
{
    clipboard->register_interest(clipboard_observer);
    if (auto const source = clipboard->paste_source())
    {
        paste_source_set(source);
    }
}

mf::XWaylandClipboardProvider::~XWaylandClipboardProvider()
{
    clipboard->unregister_interest(*clipboard_observer);
    xcb_destroy_window(connection, selection_window);
    connection.flush();
}

void mf::XWaylandClipboardProvider::selection_request_event(xcb_selection_request_event_t* event)
{
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("XCB_SELECTION_REQUEST");
    }

    if (event->owner != selection_window)
    {
        return;
    }

    if (event->selection == connection.CLIPBOARD_MANAGER)
    {
        // TODO: figure out what to do here
        log_warning("XCB_SELECTION_REQUEST with selection=CLIPBOARD_MANAGER (needs to be handled somehow)");
    }
    else if (event->selection != connection.CLIPBOARD)
    {
        // TODO: ignore this? or handle it?
        log_warning("Got non-clipboard selection request event");
        send_selection_notify(event->time, event->requestor, XCB_ATOM_NONE, event->selection, event->target);
    }
    else if (event->target == connection.TARGETS)
    {
        send_targets(event->time, event->requestor, event->property);
    }
    else if (event->target == connection.TIMESTAMP)
    {
        send_timestamp(event->time, event->requestor, event->property);
    }
    else if (event->target == connection.UTF8_STRING || event->target == connection.TEXT)
    {
        send_data(event->time, event->requestor, event->property, event->target, "text/plain;charset=utf-8");
    }
    else
    {
        // This seems to happen during normal operation
        if (verbose_xwayland_logging_enabled())
        {
            log_info(
                "XWayland selection request event with invalid target %s",
                connection.query_name(event->target).c_str());
        }
        send_selection_notify(event->time, event->requestor, XCB_ATOM_NONE, event->selection, event->target);
    }
}

void mf::XWaylandClipboardProvider::xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event)
{
    if (event->owner == selection_window && event->selection == connection.CLIPBOARD)
    {
        // We have to use XCB_TIME_CURRENT_TIME when we claim the selection, so grab the actual timestamp here so we can
        // answer TIMESTAMP conversion requests correctly (this is what Weston does)
        std::lock_guard<std::mutex> lock{mutex};
        clipboard_ownership_timestamp = event->timestamp;
    }
}

void mf::XWaylandClipboardProvider::send_targets(
    xcb_timestamp_t time,
    xcb_window_t requester,
    xcb_atom_t property)
{
    xcb_atom_t const targets[] = {
        connection.TIMESTAMP,
        connection.TARGETS,
        connection.UTF8_STRING,
        connection.TEXT,
    };

    connection.set_property<XCBType::ATOM>(requester, property, targets);
    send_selection_notify(time, requester, property, connection.CLIPBOARD, connection.TARGETS);
}

void mf::XWaylandClipboardProvider::send_timestamp(
    xcb_timestamp_t time,
    xcb_window_t requester,
    xcb_atom_t property)
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        connection.set_property<XCBType::INTEGER>(requester, property, clipboard_ownership_timestamp);
    }
    send_selection_notify(time, requester, property, connection.CLIPBOARD, connection.TIMESTAMP);
}

void mf::XWaylandClipboardProvider::send_data(
    xcb_timestamp_t time,
    xcb_window_t requester,
    xcb_atom_t property,
    xcb_atom_t target,
    std::string const& mime_type)
{
    int fds[2];

    if (pipe2(fds, O_CLOEXEC) != 0)
    {
        log_warning("failed to send clipboard data to X11 client: pipe2 error: %s", strerror(errno));
        send_selection_notify(time, requester, XCB_ATOM_NONE, connection.CLIPBOARD, target);
        return;
    }

    Fd out_fd{fds[0]}, in_fd{fds[1]};

    {
        std::lock_guard<std::mutex> lock{mutex};
        if (!current_source)
        {
            log_warning("failed to send clipboard data to X11 client: no source");
            send_selection_notify(time, requester, XCB_ATOM_NONE, connection.CLIPBOARD, target);
            return;
        }
        current_source->initiate_send(mime_type, in_fd);
        in_fd = {}; // important to release ownership of the fd, as we are about to block on it closing
    }

    size_t const data_size = INCREMENT_CHUNK_SIZE;
    std::unique_ptr<uint8_t[]> const data{new uint8_t[data_size]};

    size_t len;
    try
    {
        len = read_repeatedly(out_fd, data.get(), data_size);
    }
    catch (ReadError const& err)
    {
        log_warning("failed to send clipboard data to X11 client: failed to read from fd: %s", err.what());
        send_selection_notify(time, requester, XCB_ATOM_NONE, connection.CLIPBOARD, target);
        return;
    }

    if (len < data_size)
    {
        // Call the XCB function directly instead of using connection->set_property() because target is variable
        xcb_change_property(
            connection,
            XCB_PROP_MODE_REPLACE,
            requester,
            property,
            target,
            8, // format
            len,
            data.get());
        send_selection_notify(time, requester, property, connection.CLIPBOARD, target);
    }
    else
    {
        log_warning("failed to send clipboard data to X11 client: incremental transfers not yet implemented");
        send_selection_notify(time, requester, XCB_ATOM_NONE, connection.CLIPBOARD, target);
        // TODO: remember to flush the connection before reading
        return;
    }

    xcb_flush(connection);
}

void mf::XWaylandClipboardProvider::send_selection_notify(
    xcb_timestamp_t time,
    xcb_window_t requestor,
    xcb_atom_t property,
    xcb_atom_t selection,
    xcb_atom_t target)
{
    xcb_selection_notify_event_t const selection_notify = {
        XCB_SELECTION_NOTIFY, // response_type
        0, // pad0
        0, // sequence
        time,
        requestor,
        selection,
        target,
        property,
    };

    xcb_send_event(
        connection,
        0, // ropagate
        requestor,
        XCB_EVENT_MASK_NO_EVENT,
        reinterpret_cast<const char*>(&selection_notify));
}

void mf::XWaylandClipboardProvider::paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source)
{
    std::unique_lock<std::mutex> lock{mutex};

    if (XWaylandClipboardSource::source_is_from(source.get(), connection))
    {
        // If the source is from our XWayland connection, clear current_source and don't touch the X11 selection owner
        current_source = nullptr;
    }
    else
    {
        current_source = source;
        if (source)
        {
            // We have a source that isn't from this XWayland connection. Take ownership of the X11 clipboard. We don't
            // try to prevent calling this multiple when we already have ownership.
            if (verbose_xwayland_logging_enabled())
            {
                log_debug("Taking ownership of the XWayland clipboard");
            }
            xcb_set_selection_owner(connection, selection_window, connection.CLIPBOARD, XCB_TIME_CURRENT_TIME);
        }
        else
        {
            // Clear ownership of the X11 clipboard
            if (verbose_xwayland_logging_enabled())
            {
                log_debug("Clearing our ownership of the XWayland clipboard");
            }
            xcb_set_selection_owner(connection, XCB_WINDOW_NONE, connection.CLIPBOARD, clipboard_ownership_timestamp);
        }
    }

    lock.unlock();

    connection.flush();
}
