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
}

class mf::XWaylandClipboardProvider::SelectionSender
    : public md::Dispatchable,
      public std::enable_shared_from_this<SelectionSender>
{
public:
    SelectionSender(
        std::shared_ptr<mf::XCBConnection> const& connection,
        std::shared_ptr<ThreadsafeSelf> const& provider,
        Fd&& source_fd,
        xcb_timestamp_t time,
        xcb_window_t requester,
        xcb_atom_t selection,
        xcb_atom_t property,
        xcb_atom_t target)
        : connection{connection},
          provider{provider},
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

    auto watch_fd() const -> Fd override
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
            ssize_t len = 0;
            if (free_space > 0)
            {
                len = read(source_fd, buffer.get() + data_size, free_space);
                if (len < 0)
                {
                    // Error reading from fd
                    notify_cancelled(strerror(errno));
                    return false;
                }
                data_size += len;
            }

            if (len == 0)
            {
                // We've either hit EOF or filled up our buffer

                if (incremental_transfer_in_progress)
                {
                    // Time to send more data.
                    progress_incremental_transfer();
                }
                else if (free_space == 0)
                {
                    // We filled up the buffer. Long buffers need to use incremental transfers.
                    initiate_incremental_transfer();
                }
                else
                {
                    // We hit EOF before filling up the buffer, so we send all data to the X11 client in one go
                    send_all_data_in_single_transfer();
                }

                // in any case, we don't want to keep reading
                return false;
            }

            // If we haven't filled up the buffer or hit EOF, keep reading by returning true
        }

        return true;
    }

    auto relevant_events() const -> md::FdEvents override
    {
        return md::FdEvent::readable;
    }

    void send_all_data_in_single_transfer()
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
        notify_sent();
        connection->flush();
    }

    void initiate_incremental_transfer()
    {
        std::lock_guard lock{provider->mutex};
        if (provider->ptr)
        {
            // https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#incr_properties:
            // "The contents of the INCR property will be an integer, which represents a lower bound on the number of bytes
            // of data in the selection"
            //
            // data_size is how much data we've read so far, which is such a lower bound
            connection->set_property<XCBType::INCR>(requester, property, static_cast<int32_t>(data_size));
            notify_sent();
            connection->flush();
            incremental_transfer_in_progress = true;
            provider->ptr->defer_incremental_send(shared_from_this(), requester, property);
            // The data is left in the buffer unsent. When the property is deleted by the client, the provider will add
            // us back to the dispatcher. On the first fd readable notification we will notice our buffer is full and
            // proceed to send it.
        }
        else
        {
            notify_cancelled("clipboard provider destroyed before incremental transfer initiated");
        }
    }

    void progress_incremental_transfer()
    {
        // Call the XCB function directly instead of using connection->set_property() because target is variable and the
        // spec says to append instead of replace (although both seem to work fine)
        xcb_change_property(
            *connection,
            XCB_PROP_MODE_APPEND,
            requester,
            property,
            target,
            8, // format
            data_size,
            buffer.get());
        connection->flush();
        bool const still_sending = data_size > 0;
        data_size = 0;

        if (still_sending)
        {
            std::lock_guard lock{provider->mutex};
            if (provider->ptr)
            {
                provider->ptr->defer_incremental_send(shared_from_this(), requester, property);
            }
            else
            {
                log_warning("failed to finish incremental X11 data transfer because provider was destroyed");
            }
        }
        // Once we send a zero-size chunk we're done!
    }

    void notify_sent() const
    {
        send_selection_notify(*connection, time, requester, property, selection, target);
    }

    void notify_cancelled(const char* reason) const
    {
        mir::log_warning("failed to send clipboard data to X11 client: %s", reason);
        send_selection_notify(*connection, time, requester, XCB_ATOM_NONE, selection, target);
        connection->flush();
    }

    std::shared_ptr<mf::XCBConnection> const connection;
    std::shared_ptr<ThreadsafeSelf> const provider;
    mir::Fd const source_fd;
    xcb_timestamp_t const time;
    xcb_window_t const requester;
    xcb_atom_t const selection;
    xcb_atom_t const property;
    xcb_atom_t const target;
    size_t const buffer_size; ///< the size of the allocated memory
    bool incremental_transfer_in_progress = false;
    size_t data_size; ///< how much of the buffer is being used
    std::unique_ptr<uint8_t[]> const buffer;
};

class mf::XWaylandClipboardProvider::ClipboardObserver : public scene::ClipboardObserver
{
public:
    ClipboardObserver(XWaylandClipboardProvider* const owner)
        : owner{owner}
    {
    }

    void paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source) override
    {
        owner->paste_source_set(source);
    }

private:
    void drag_n_drop_source_set(const std::shared_ptr<ms::DataExchangeSource>&) override {}
    void drag_n_drop_source_cleared(const std::shared_ptr<ms::DataExchangeSource>&) override {}
    void end_of_dnd_gesture() override {}

    XWaylandClipboardProvider* const owner;
};

mf::XWaylandClipboardProvider::XWaylandClipboardProvider(
    std::shared_ptr<XCBConnection> const& connection,
    std::shared_ptr<md::MultiplexingDispatchable> const& dispatcher,
    std::shared_ptr<scene::Clipboard> const& clipboard)
    : threadsafe_self{std::make_shared<ThreadsafeSelf>(this)},
      connection{connection},
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
    {
        std::lock_guard lock{threadsafe_self->mutex};
        threadsafe_self->ptr = nullptr;
    }
    if (!pending_incremental_sends.empty())
    {
        log_warning(
            "XWaylandClipboardProvider destroyed with %zu incremental sends in progress",
            pending_incremental_sends.size());
    }
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
        std::lock_guard lock{mutex};
        clipboard_ownership_timestamp = event->timestamp;
    }
}

void mf::XWaylandClipboardProvider::property_deleted_event(xcb_window_t window, xcb_atom_t property)
{
    auto const window_prop = std::make_pair(window, property);
    auto const iter = pending_incremental_sends.find(window_prop);
    if (iter != pending_incremental_sends.end())
    {
        auto sender = std::move(iter->second);
        pending_incremental_sends.erase(iter);
        dispatcher->add_watch(std::move(sender));
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
        std::lock_guard lock{mutex};
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

        std::lock_guard lock{mutex};
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
        threadsafe_self,
        std::move(out_fd),
        time,
        requester,
        connection->CLIPBOARD,
        property,
        target));
}

void mf::XWaylandClipboardProvider::paste_source_set(std::shared_ptr<ms::DataExchangeSource> const& source)
{
    std::unique_lock lock{mutex};

    if (XWaylandClipboardSource::source_is_from(source.get(), *connection))
    {
        // If the source is from our XWayland connection, clear current_source and don't touch the X11 selection target
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

void mf::XWaylandClipboardProvider::defer_incremental_send(
    std::shared_ptr<SelectionSender> sender,
    xcb_window_t window,
    xcb_atom_t property)
{
    auto const window_prop = std::make_pair(window, property);
    pending_incremental_sends[window_prop] = std::move(sender);
}
