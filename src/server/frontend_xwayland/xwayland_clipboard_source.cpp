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

#include "xwayland_clipboard_source.h"

#include "xwayland_log.h"
#include "mir/scene/clipboard.h"
#include "mir/dispatch/multiplexing_dispatchable.h"

#include <xcb/xfixes.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
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

class mf::XWaylandClipboardSource::ClipboardSource : public ms::DataExchangeSource
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
        std::lock_guard lock{mutex};
        owner = nullptr;
    }

    auto mime_types() const -> std::vector<std::string> const& override
    {
        return mime_types_vec;
    }

    void initiate_send(std::string const& mime_type, Fd const& receiver_fd) override
    {
        std::lock_guard lock{mutex};
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
    void cancelled() override {}
    void dnd_drop_performed() override {}
    auto actions() -> uint32_t override { return 0; }
    void offer_accepted(const std::optional<std::string>&) override {}
    uint32_t offer_set_actions(uint32_t, uint32_t) override { return 0; }
    void dnd_finished() override {}

    std::vector<std::string> const mime_types_vec;
    std::map<std::string, xcb_atom_t> const mime_types_map;
    std::mutex mutex;
    XWaylandClipboardSource* owner; ///< Can be null
};

class mf::XWaylandClipboardSource::DataSender : public md::Dispatchable
{
public:
    DataSender(mir::Fd const& destination_fd)
        : destination_fd{destination_fd}
    {
    }

    /// Returns if the previous buffer was empty. If return value is true, this needs to be added to the dispatcher.
    auto add_data(std::vector<uint8_t>&& new_data) -> bool {
        std::lock_guard lock{mutex};
        if (data.empty())
        {
            data = std::move(new_data);
            return true;
        }
        else
        {
            data.reserve(data.size() + new_data.size());
            std::copy(new_data.begin(), new_data.end(), std::back_inserter(data));
            return false;
        }
    }

private:
    auto watch_fd() const -> mir::Fd override
    {
        return destination_fd;
    }

    auto dispatch(md::FdEvents events) -> bool override
    {
        std::lock_guard lock{mutex};

        if (events & md::FdEvent::error)
        {
            mir::log_error("failed to send X11 clipboard data: fd error");
            return false;
        }

        if (events & md::FdEvent::remote_closed)
        {
            mir::log_error("failed to send X11 clipboard data: fd closed");
            return false;
        }

        if (events & md::FdEvent::writable)
        {
            auto const len = write(destination_fd, &data[0], data.size());
            if (len < 0)
            {
                mir::log_error("failed to send X11 clipboard data: %s", strerror(errno));
                return false;
            }
            data.erase(data.begin(), data.begin() + len);
        }

        return !data.empty();
    }

    auto relevant_events() const -> md::FdEvents override
    {
        return md::FdEvent::writable;
    }

    mir::Fd const destination_fd;

    std::mutex mutex;
    std::vector<uint8_t> data;
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

auto mf::XWaylandClipboardSource::source_is_from(ms::DataExchangeSource* source, XCBConnection& connection) -> bool
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
    std::unique_lock lock{mutex};
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
    std::unique_lock lock{mutex};
    if (in_progress_send)
    {
        log_error("can not send clipboard data from X11 because another send is currently in progress");
        return;
    }
    in_progress_send = std::make_shared<DataSender>(receiver_fd);
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

    // Needed here because this is not called during an X11 event, and so won't get flushed at the end
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
        // now read it out and give it to in_progress_send, which will write it to the receiver fd

        if (verbose_xwayland_logging_enabled())
        {
            log_info("Clipboard data from X11 client is ready");
        }

        std::lock_guard lock{mutex};
        read_and_send_wl_selection_data(lock);
    }
}

void mf::XWaylandClipboardSource::xfixes_selection_notify_event(xcb_xfixes_selection_notify_event_t* event)
{
    if (event->selection != connection.CLIPBOARD) {
        // Ignore events pertaining to selections other than CLIPBOARD
        return;
    }

    std::unique_lock lock{mutex};

    current_clipbaord_owner = event->owner;
    clipboard_ownership_timestamp = event->timestamp;
    // No matter what, our old source (if any) is no longer valid
    auto const source_to_reset = std::move(clipboard_source);

    // Only start the process if the new selection target is a window not created by us
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
            log_info("Invalidating X11 clipboard source");
        }
        source_to_reset->invalidate_owner();
    }
}

void mf::XWaylandClipboardSource::property_notify_event(xcb_window_t window, xcb_atom_t property)
{
    if (window != receiving_window || property != connection._WL_SELECTION)
    {
        return;
    }

    std::lock_guard lock{mutex};
    if (incremental_transfer_in_progress)
    {
        read_and_send_wl_selection_data(lock);
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

    std::unique_lock lock{mutex};
    if (clipboard_ownership_timestamp != timestamp)
    {
        // Something has happened since we requested the targets
        return;
    }
    clipboard_source = source;
    lock.unlock();

    clipboard->set_paste_source(source);
}

void mf::XWaylandClipboardSource::read_and_send_wl_selection_data(std::lock_guard<std::mutex> const& lock)
{
    auto const completion = connection.read_property(
        receiving_window,
        connection._WL_SELECTION,
        true, // delete
        0x1fffffff, // length lifted from Weston
        {[&](xcb_get_property_reply_t* reply)
        {
            if (reply->type == connection.INCR)
            {
                if (verbose_xwayland_logging_enabled())
                {
                    log_info("Initiating incremental data transfer from X11");
                }
                incremental_transfer_in_progress = true;
            }
            else
            {
                auto const data_ptr = static_cast<uint8_t*>(xcb_get_property_value(reply));
                auto const data_size = xcb_get_property_value_length(reply);
                add_data_to_in_progress_send(lock, data_ptr, data_size);
            }
        },
        [&](const std::string& error_message)
        {
            log_error("Error getting selection property: %s", error_message.c_str());
            in_progress_send.reset();
        }});

    completion();
}

void mf::XWaylandClipboardSource::add_data_to_in_progress_send(
    std::lock_guard<std::mutex> const&,
    uint8_t* data_ptr,
    size_t data_size)
{

    if (!in_progress_send)
    {
        log_error("Can not send clipboard data from X11 because there is no send in progress");
        return;
    }

    if (data_size > 0)
    {
        std::vector<uint8_t> data{data_ptr, data_ptr + data_size};
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Writing %zu bytes of clipboard data from X11", data.size());
        }

        if (in_progress_send->add_data(std::move(data)))
        {
            // add_data() returns if it needs to be added to the dispatcher
            dispatcher->add_watch(in_progress_send);
        }
    }

    // Normal transfers are done after the first chunk, incremental transfers are done after a zero-size chunk
    if (!incremental_transfer_in_progress || data_size == 0)
    {
        // in_progress_send may still be sending data on it's fd, but the dispatcher will hold onto it until it's done
        in_progress_send.reset();
        incremental_transfer_in_progress = false;
    }
}
