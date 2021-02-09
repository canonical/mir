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
#include "mir/dispatch/multiplexing_dispatchable.h"

#include <xcb/xfixes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace md = mir::dispatch;

namespace
{
size_t const increment_chunk_size = 64 * 1024;

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

void send_selection_notify(
    mf::XCBConnection& connection,
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
        0, // propagate
        requestor,
        XCB_EVENT_MASK_NO_EVENT,
        reinterpret_cast<const char*>(&selection_notify));
}

class SelectionSender : public md::Dispatchable
{
public:
    SelectionSender(
        std::shared_ptr<mf::XCBConnection> const& connection,
        mir::Fd&& source_fd,
        xcb_timestamp_t time,
        xcb_window_t requester,
        xcb_atom_t selection,
        xcb_atom_t property,
        xcb_atom_t target)
        : connection{connection},
          source_fd{std::move(source_fd)},
          time{time},
          requester{requester},
          selection{selection},
          property{property},
          target{target},
          buffer_size{increment_chunk_size},
          data_size{0},
          buffer{new uint8_t[buffer_size]}
    {
    }

private:

    auto watch_fd() const -> mir::Fd override
    {
        return source_fd;
    }

    auto dispatch(md::FdEvents events) -> bool override
    {
        if (events & md::FdEvent::error)
        {
            notify_cancelled("fd error");
            return false;
        }

        if (events & md::FdEvent::readable || events & md::FdEvent::remote_closed)
        {
            auto const free_space = buffer_size - data_size;
            auto const len = read(source_fd, buffer.get() + data_size, free_space);
            if (len < 0)
            {
                // Error reading from fd
                notify_cancelled(strerror(errno));
                return false;
            }
            data_size += len;
            if (len == 0)
            {
                // EOF, so we send it to the X11 client
                send_data();
                return false;
            }
            if (data_size == free_space)
            {
                // We filled up the buffer. Long buffers need to use incremental transfers, but we don't support that.'
                notify_cancelled("incremental paste not yet supported");
                return false;
            }
        }

        return true;
    }

    auto relevant_events() const -> md::FdEvents override
    {
        return md::FdEvent::readable;
    }

    void send_data()
    {
        // Call the XCB function directly instead of using connection->set_property() because target is variable
        xcb_change_property(
            *connection,
            XCB_PROP_MODE_REPLACE,
            requester,
            property,
            target,
            8, // format
            data_size,
            buffer.get());
        send_selection_notify(*connection, time, requester, property, selection, target);
        connection->flush();
    }

    void notify_cancelled(const char* reason) const
    {
        mir::log_warning("failed to send clipboard data to X11 client: %s", reason);
        send_selection_notify(*connection, time, requester, XCB_ATOM_NONE, selection, target);
        connection->flush();
    }

    std::shared_ptr<mf::XCBConnection> const connection;
    mir::Fd const source_fd;
    xcb_timestamp_t const time;
    xcb_window_t const requester;
    xcb_atom_t const selection;
    xcb_atom_t const property;
    xcb_atom_t const target;
    size_t const buffer_size; ///< the size of the allocated memory
    size_t data_size; ///< how much of the buffer is being used
    std::unique_ptr<uint8_t[]> const buffer;
};
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
    std::shared_ptr<XCBConnection> const& connection,
    std::shared_ptr<md::MultiplexingDispatchable> const& dispatcher,
    std::shared_ptr<scene::Clipboard> const& clipboard)
    : connection{connection},
      dispatcher{dispatcher},
      clipboard{clipboard},
      clipboard_observer{std::make_shared<ClipboardObserver>(this)},
      selection_window{create_selection_window(*connection)}
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
    xcb_destroy_window(*connection, selection_window);
    connection->flush();
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

    if (event->selection == connection->CLIPBOARD_MANAGER)
    {
        // TODO: figure out what to do here
        log_warning("XCB_SELECTION_REQUEST with selection=CLIPBOARD_MANAGER (needs to be handled somehow)");
    }
    else if (event->selection != connection->CLIPBOARD)
    {
        // TODO: ignore this? or handle it?
        log_warning("Got non-clipboard selection request event");
        send_selection_notify(*connection, event->time, event->requestor, XCB_ATOM_NONE, event->selection, event->target);
    }
    else if (event->target == connection->TARGETS)
    {
        send_targets(event->time, event->requestor, event->property);
    }
    else if (event->target == connection->TIMESTAMP)
    {
        send_timestamp(event->time, event->requestor, event->property);
    }
    else if (event->target == connection->UTF8_STRING || event->target == connection->TEXT)
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
                connection->query_name(event->target).c_str());
        }
        send_selection_notify(*connection, event->time, event->requestor, XCB_ATOM_NONE, event->selection, event->target);
    }
}

void mf::XWaylandClipboardProvider::xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event)
{
    if (event->owner == selection_window && event->selection == connection->CLIPBOARD)
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
        connection->TIMESTAMP,
        connection->TARGETS,
        connection->UTF8_STRING,
        connection->TEXT,
    };

    connection->set_property<XCBType::ATOM>(requester, property, targets);
    send_selection_notify(*connection, time, requester, property, connection->CLIPBOARD, connection->TARGETS);
}

void mf::XWaylandClipboardProvider::send_timestamp(
    xcb_timestamp_t time,
    xcb_window_t requester,
    xcb_atom_t property)
{
    {
        std::lock_guard<std::mutex> lock{mutex};
        // Unclear why the timestamp (which has an unsigned type in our code) is sent with an integer (signed) type
        // instead of a cardinal (unsigned) type, but that's what Weston does.
        connection->set_property<XCBType::INTEGER32>(
            requester,
            property,
            static_cast<int32_t>(clipboard_ownership_timestamp));
    }
    send_selection_notify(*connection, time, requester, property, connection->CLIPBOARD, connection->TIMESTAMP);
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
        send_selection_notify(*connection, time, requester, XCB_ATOM_NONE, connection->CLIPBOARD, target);
        return;
    }

    Fd out_fd{fds[0]};

    {
        Fd in_fd{fds[1]};

        std::lock_guard<std::mutex> lock{mutex};
        if (!current_source)
        {
            log_warning("failed to send clipboard data to X11 client: no source");
            send_selection_notify(*connection, time, requester, XCB_ATOM_NONE, connection->CLIPBOARD, target);
            return;
        }
        current_source->initiate_send(mime_type, std::move(in_fd));
    }

    dispatcher->add_watch(std::make_shared<SelectionSender>(
        connection,
        std::move(out_fd),
        time,
        requester,
        connection->CLIPBOARD,
        property,
        target));
}

void mf::XWaylandClipboardProvider::paste_source_set(std::shared_ptr<ms::ClipboardSource> const& source)
{
    std::unique_lock<std::mutex> lock{mutex};

    if (XWaylandClipboardSource::source_is_from(source.get(), *connection))
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
            xcb_set_selection_owner(*connection, selection_window, connection->CLIPBOARD, XCB_TIME_CURRENT_TIME);
        }
        else
        {
            // Clear ownership of the X11 clipboard
            if (verbose_xwayland_logging_enabled())
            {
                log_debug("Clearing our ownership of the XWayland clipboard");
            }
            xcb_set_selection_owner(*connection, XCB_WINDOW_NONE, connection->CLIPBOARD, clipboard_ownership_timestamp);
        }
    }

    lock.unlock();

    connection->flush();
}
