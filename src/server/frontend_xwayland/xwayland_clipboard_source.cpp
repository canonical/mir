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

#include "xwayland_clipboard_source.h"

#include "xwayland_log.h"
#include "mir/scene/clipboard.h"
#include "mir/dispatch/multiplexing_dispatchable.h"

#include <xcb/xfixes.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <set>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace md = mir::dispatch;

namespace
{
auto create_receiving_window(mf::XCBConnection const& connection) -> xcb_window_t
{
    uint32_t const attrib_values[]{XCB_EVENT_MASK_PROPERTY_CHANGE};

    xcb_window_t const receiving_window = xcb_generate_id(connection);
    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        receiving_window,
        connection.root_window(),
        0, 0, 10, 10, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        connection.screen()->root_visual,
        XCB_CW_EVENT_MASK, attrib_values);

    xcb_set_selection_owner(connection, receiving_window, connection.CLIPBOARD_MANAGER, XCB_TIME_CURRENT_TIME);

    uint32_t const mask =
        XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
        XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
    xcb_xfixes_select_selection_input(connection, receiving_window, connection.CLIPBOARD, mask);

    return receiving_window;
}

class WriteError : public std::runtime_error
{
public:
    WriteError() : runtime_error{strerror(errno)}
    {
    }
};

/// Like write(), but calls write() until the whole buffer has been written
void write_repeatedly(mir::Fd const& fd, uint8_t const* buffer, size_t size)
{
    auto remaining = size;
    while (remaining > 0)
    {
        auto len = write(fd, buffer, remaining);
        if (len < 0)
        {
            throw WriteError();
        }
        buffer += len;
        remaining -= len;
    }
}

auto map2vec(std::map<std::string, xcb_atom_t> const& map) -> std::vector<std::string>
{
    std::vector<std::string> result;
    for (auto const& i : map)
    {
        result.push_back(i.first);
    }
    return result;
}
}

class mf::XWaylandClipboardSource::ClipboardSource : public scene::ClipboardSource
{
public:
    ClipboardSource(std::map<std::string, xcb_atom_t>&& mime_types_map, XWaylandClipboardSource* owner)
        : connection{owner->connection},
          mime_types_vec{map2vec(mime_types_map)},
          mime_types_map{mime_types_map},
          owner{owner}
    {
    }

    void invalidate_owner()
    {
        std::lock_guard<std::mutex> lock{mutex};
        owner = nullptr;
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        return mime_types_vec;
    }

    void initiate_send(std::string const& mime_type, Fd const& receiver_fd) override
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (owner)
        {
            auto const target_type = mime_types_map.find(mime_type);
            if (target_type == mime_types_map.end())
            {
                log_error("X11 clipboard source given invalid mime type %s", mime_type.c_str());
                return;
            }
            owner->initiate_send(target_type->second, receiver_fd);
        }
    }

    /// Only used to check equality in XWaylandClipboardSource::source_is_from()
    void* const connection;

private:
    std::vector<std::string> const mime_types_vec;
    std::map<std::string, xcb_atom_t> const mime_types_map;
    std::mutex mutex;
    XWaylandClipboardSource* owner; ///< Can be null
};

mf::XWaylandClipboardSource::XWaylandClipboardSource(
    XCBConnection& connection,
    std::shared_ptr<md::MultiplexingDispatchable> const& dispatcher,
    std::shared_ptr<scene::Clipboard> const& clipboard)
    : connection{connection},
      dispatcher{dispatcher},
      clipboard{clipboard},
      receiving_window{create_receiving_window(connection)}
{
}

auto mf::XWaylandClipboardSource::source_is_from(ms::ClipboardSource* source, XCBConnection& connection) -> bool
{
    if (auto const xwayland_source = dynamic_cast<ClipboardSource*>(source))
    {
        return xwayland_source->connection == connection;
    }
    else
    {
        return false;
    }

}

mf::XWaylandClipboardSource::~XWaylandClipboardSource()
{
    std::unique_lock<std::mutex> lock{mutex};
    auto const source_to_reset = std::move(clipboard_source);
    lock.unlock();

    if (source_to_reset)
    {
        clipboard->clear_paste_source(*source_to_reset);
        // Because threadsafety, we shouldn't assume that no more callbacks will come from the source, so we explicitly
        // invalidate it. It's important our mutex isn't locked while doing this.
        source_to_reset->invalidate_owner();
    }

    xcb_destroy_window(connection, receiving_window);
    connection.flush();
}

void mf::XWaylandClipboardSource::initiate_send(xcb_atom_t target_type, Fd const& receiver_fd)
{
    std::unique_lock<std::mutex> lock{mutex};
    if (pending_send_fd)
    {
        log_error("can not send clipboard data from X11 because another send is currently in progress");
        return;
    }
    pending_send_fd = receiver_fd;
    lock.unlock();

    if (verbose_xwayland_logging_enabled())
    {
        log_info("Initiating clipboard data send from X11");
    }

    // This will cause the client to load the data into receiving_window._WL_SELECTION
    xcb_convert_selection(
        connection,
        receiving_window,
        connection.CLIPBOARD,
        target_type,
        connection._WL_SELECTION,
        XCB_TIME_CURRENT_TIME);

    // Needed here because this is not called during an X11 event, and so wont get flushed at the end
    connection.flush();
}

void mf::XWaylandClipboardSource::selection_notify_event(xcb_selection_notify_event_t* event)
{
    if (event->property != connection._WL_SELECTION)
    {
        // Convert selection failed
    }
    else if (event->target == connection.TARGETS)
    {
        // Before creating a source we need to know what mime types it will support. This is determined by the targets
        // the X11 client supports. Here, the targets have been loaded into a property on our window but we still need
        // to read them out.

        if (verbose_xwayland_logging_enabled())
        {
            log_info("Targets for new X11 selection are ready");
        }

        auto const completion = connection.read_property(
            receiving_window,
            connection._WL_SELECTION,
            {[&](std::vector<xcb_atom_t> const& targets)
            {
                create_source(event->time, targets);
            }});

        completion();
    }
    else
    {
        // A send from our source has been requested, and the data is now loaded into a property on our window. We will
        // now read the data out and write it to the receiver fd

        if (verbose_xwayland_logging_enabled())
        {
            log_info("Clipboard data from X11 client is ready");
        }

        auto const completion = connection.read_property(
            receiving_window,
            connection._WL_SELECTION,
            true, // delete
            0x1fffffff, // length lifted from Weston
            {[&](xcb_get_property_reply_t* reply)
            {
                if (reply->type == connection.INCR)
                {
                    log_error("Incremental copies from X11 are not supported");
                }
                else
                {
                    send_data_to_fd(
                        static_cast<uint8_t*>(xcb_get_property_value(reply)),
                        xcb_get_property_value_length(reply));
                }
            },
            [&](const std::string& error_message)
            {
                log_error("Error getting selection property: %s", error_message.c_str());
                std::lock_guard<std::mutex> lock{mutex};
                pending_send_fd.reset();
            }});

        completion();
    }
}

void mf::XWaylandClipboardSource::xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event)
{
    if (event->selection != connection.CLIPBOARD) {
        // Ignore events pertaining to selections other than CLIPBOARD
        return;
    }

    std::unique_lock<std::mutex> lock{mutex};

    current_clipbaord_owner = event->owner;
    clipboard_ownership_timestamp = event->timestamp;
    // No matter what, our old source (if any) is no longer valid
    auto const source_to_reset = std::move(clipboard_source);

    // Only start the process if the new selection owner is a window not created by us
    if (current_clipbaord_owner != XCB_WINDOW_NONE && !connection.is_ours(current_clipbaord_owner))
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Querying targets for new X11 selection");
        }

        // This will trigger a SELECTION_NOTIFY event when the targets (aka mime types) have been loaded by the sender
        // into receiving_window._WL_SELECTION
        xcb_convert_selection(
            connection,
            receiving_window,
            connection.CLIPBOARD,
            connection.TARGETS,
            connection._WL_SELECTION,
            event->timestamp);
    }

    lock.unlock();
    if (source_to_reset)
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Clearing old X11 clipboard source");
        }
        clipboard->clear_paste_source(*source_to_reset);
        source_to_reset->invalidate_owner();
    }
}

void mf::XWaylandClipboardSource::create_source(xcb_timestamp_t timestamp, std::vector<xcb_atom_t> const& targets)
{
    std::set<xcb_atom_t> target_set{targets.begin(), targets.end()};
    std::map<std::string, xcb_atom_t> mime_types;

    auto const text_mime_type{"text/plain;charset=utf-8"};
    if (target_set.find(connection.UTF8_STRING) != target_set.end())
    {
        mime_types[text_mime_type] = connection.UTF8_STRING;
    }
    else if (target_set.find(connection.TEXT) != target_set.end())
    {
        mime_types[text_mime_type] = connection.TEXT;
    }

    if (verbose_xwayland_logging_enabled())
    {
        std::string msg = "New X11 clipboard source created with mime types:";
        for (auto const& pair : mime_types)
        {
            msg += "\n    " + pair.first + ": " + connection.query_name(pair.second);
        }
        log_info(msg);
    }

    auto const source = std::make_shared<ClipboardSource>(std::move(mime_types), this);

    std::unique_lock<std::mutex> lock{mutex};
    if (clipboard_ownership_timestamp != timestamp)
    {
        // Something has happened since we requested the targets
        return;
    }
    clipboard_source = source;
    lock.unlock();

    clipboard->set_paste_source(source);
}

void mf::XWaylandClipboardSource::send_data_to_fd(uint8_t const* data, size_t size)
{
    std::unique_lock<std::mutex> lock{mutex};
    if (!pending_send_fd)
    {
        log_error("Can not send clipboard data from X11 because pending_send_fd is not set");
        return;
    }
    auto const fd = pending_send_fd.value();
    lock.unlock();

    if (verbose_xwayland_logging_enabled())
    {
        log_info("Writing clipboard data from X11");
    }

    try
    {
        write_repeatedly(fd, data, size);
    }
    catch (WriteError const& err)
    {
        log_error("Failed to write X11 clipboard data to fd: %s", err.what());
        // Continue to clear the fd either way
    }

    lock.lock();
    if (pending_send_fd && pending_send_fd.value() == fd)
    {
        pending_send_fd.reset();
    }
}
