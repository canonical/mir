/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "input_sender.h"

#include "mir/input/android/event_conversion_helpers.h"
#include "mir/input/input_channel.h"
#include "mir/input/input_report.h"
#include "mir/scene/surface.h"
#include "mir/compositor/scene.h"
#include "mir/main_loop.h"
#include "mir/events/event_private.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <cstring>
#include <algorithm>
#include <iterator>

namespace mi = mir::input;
namespace mia = mi::android;

namespace droidinput = android;

mia::InputSender::InputSender(std::shared_ptr<mir::compositor::Scene> const& scene,
                              std::shared_ptr<mir::MainLoop> const& main_loop,
                              std::shared_ptr<InputReport> const& report)
    : state{main_loop, report}, scene{scene}
{
    scene->add_observer(std::make_shared<SceneObserver>(state));
}

mia::InputSender::SceneObserver::SceneObserver(InputSenderState & state)
    : state(state)
{
}

void mia::InputSender::SceneObserver::surface_added(scene::Surface* surface)
{
    if (surface && surface->input_channel())
    {
        state.add_transfer(surface->input_channel(), surface);
    }
}

void mia::InputSender::SceneObserver::surface_removed(scene::Surface* surface)
{
    if (surface && surface->input_channel())
    {
        remove_transfer_for(surface);
    }
}

void mia::InputSender::SceneObserver::surface_exists(scene::Surface* surface)
{
    surface_added(surface);
}

void mia::InputSender::SceneObserver::scene_changed()
{
}


void mia::InputSender::SceneObserver::remove_transfer_for(mi::Surface * surface)
{
    std::shared_ptr<InputChannel> closed_channel = surface->input_channel();

    if (!closed_channel)
        return;

    state.remove_transfer(closed_channel->server_fd());

}

mia::InputSender::InputSenderState::InputSenderState(std::shared_ptr<mir::MainLoop> const& main_loop,
                                                     std::shared_ptr<InputReport> const& report)
    : main_loop{main_loop}, report{report}, seq{}
{
}

void mia::InputSender::send_event(MirEvent const& event, std::shared_ptr<InputChannel> const& channel)
{
    state.send_event(channel, event);
}

std::shared_ptr<mia::InputSender::ActiveTransfer> mia::InputSender::InputSenderState::get_transfer(int fd)
{
    auto pos = transfers.find(fd);

    if (pos == transfers.end())
        return nullptr;

    return pos->second;
}

void mia::InputSender::InputSenderState::send_event(std::shared_ptr<InputChannel> const& channel, MirEvent const& event)
{
    std::unique_lock<std::mutex> lock(sender_mutex);
    auto transfer = get_transfer(channel->server_fd());

    if (!transfer)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failure sending input event : Unknown channel provided"));

    lock.unlock();

    transfer->send(next_seq(), event);
}

void mia::InputSender::InputSenderState::add_transfer(std::shared_ptr<mir::input::InputChannel> const& channel, mi::Surface * surface)
{
    std::lock_guard<std::mutex> lock(sender_mutex);
    std::shared_ptr<ActiveTransfer> transfer{get_transfer(channel->server_fd())};

    if (transfer && transfer->used_for_surface(surface))
        return;

    transfers[channel->server_fd()] = std::make_shared<ActiveTransfer>(*this, channel, surface);
}

void mia::InputSender::InputSenderState::remove_transfer(int fd)
{
    std::unique_lock<std::mutex> lock(sender_mutex);
    auto transfer = get_transfer(fd);

    if (transfer)
    {
        transfer->unsubscribe();
        transfers.erase(fd);
        lock.unlock();
    }
}

uint32_t mia::InputSender::InputSenderState::next_seq()
{
    while(!++seq);
    return seq;
}

mia::InputSender::ActiveTransfer::ActiveTransfer(InputSenderState & state, std::shared_ptr<InputChannel> const& channel, mi::Surface* surface) :
    state(state),
    publisher{droidinput::sp<droidinput::InputChannel>(
            new droidinput::InputChannel(droidinput::String8(surface->name()), channel->server_fd()))},
    surface{surface},
    channel{channel}
{
}

void mia::InputSender::ActiveTransfer::send(uint32_t sequence_id, MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return;

    droidinput::status_t error_status;

    auto event_time = mir_input_event_get_event_time(mir_event_get_input_event(&event));
    auto input_event = mir_event_get_input_event(&event);
    switch(mir_input_event_get_type(input_event))
    {
    case mir_input_event_type_key:
        error_status = send_key_event(sequence_id, event);
        state.report->published_key_event(channel->server_fd(), sequence_id, event_time);
        break;
    case mir_input_event_type_touch:
        error_status = send_touch_event(sequence_id, event);
        state.report->published_motion_event(channel->server_fd(), sequence_id, event_time);
        break;
    case mir_input_event_type_pointer:
        error_status = send_pointer_event(sequence_id, event);
        state.report->published_motion_event(channel->server_fd(), sequence_id, event_time);
        break;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("unknown input event type"));
    }

    switch(error_status)
    {
    case droidinput::OK:
        subscribe();
        break;
    case droidinput::WOULD_BLOCK:
        // Not an error. Clients are allowed to ignore input.
        break;
    case droidinput::DEAD_OBJECT:
        // XXX This throw was recently added but is missing tests:
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::runtime_error("Client channel dead : ")) << boost::errinfo_errno(errno));
        break;
    default:
        BOOST_THROW_EXCEPTION(boost::enable_error_info(std::runtime_error("Failure sending input event : ")) << boost::errinfo_errno(errno));
    }
}

mia::InputSender::ActiveTransfer::~ActiveTransfer()
{
    unsubscribe();
}

void mia::InputSender::ActiveTransfer::unsubscribe()
{
    bool expected = true;
    if (std::atomic_compare_exchange_strong(&subscribed, &expected, false))
        state.main_loop->unregister_fd_handler(this);
}

void mia::InputSender::ActiveTransfer::subscribe()
{
    bool expected = false;
    if (std::atomic_compare_exchange_strong(&subscribed, &expected, true))
        state.main_loop->register_fd_handler(
            {publisher.getChannel()->getFd()},
            this,
            [this](int)
            {
                on_finish_signal();
            });
}

droidinput::status_t mia::InputSender::ActiveTransfer::send_key_event(uint32_t seq, MirEvent const& event)
{
    int32_t repeat_count = 0;
    auto input_event = mir_event_get_input_event(&event);
    auto key_event = mir_input_event_get_keyboard_event(input_event);
    auto const android_action = mia::android_keyboard_action_from_mir(repeat_count, mir_keyboard_event_action(key_event));
    std::chrono::nanoseconds const event_time{mir_input_event_get_event_time(input_event)};
    auto const flags = 0;
    return publisher.publishKeyEvent(seq,
                                     mir_input_event_get_device_id(input_event),
                                     AINPUT_SOURCE_KEYBOARD,
                                     android_action,
                                     flags,
                                     mir_keyboard_event_key_code(key_event),
                                     mir_keyboard_event_scan_code(key_event),
                                     mia::android_modifiers_from_mir(mir_keyboard_event_modifiers(key_event)),
                                     repeat_count,
                                     event.key.cookie,
                                     event_time,
                                     event_time);
}

droidinput::status_t mia::InputSender::ActiveTransfer::send_touch_event(uint32_t seq, MirEvent const& event)
{
    droidinput::status_t ret = droidinput::OK;
    droidinput::PointerCoords coords[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    droidinput::PointerProperties properties[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    auto input_event = mir_event_get_input_event(&event);
    auto touch = mir_input_event_get_touch_event(input_event);
    std::chrono::nanoseconds const event_time{mir_input_event_get_event_time(input_event)};
    auto const x_offset = 0.0f;
    auto const y_offset = 0.0f;
    auto const x_precision = 0;
    auto const y_precision = 0;
    auto const flags = 0;
    auto const edge_flags = 0;
    auto const button_state = 0;

    struct StateChange
    {
        int android_action;
        size_t index;
    };

    std::vector<StateChange> state_changes;
    for (size_t i = 0, e = mir_touch_event_point_count(touch); i != e; ++i)
    {
        auto const action = mir_touch_event_action(touch, i);
        if (action == mir_touch_action_down)
            state_changes.push_back(StateChange{AMOTION_EVENT_ACTION_DOWN, i});
        if (action == mir_touch_action_up)
            state_changes.push_back(StateChange{AMOTION_EVENT_ACTION_UP, i});
    }

    if (state_changes.empty())
        state_changes.push_back(StateChange{AMOTION_EVENT_ACTION_MOVE, 0});

    for (auto state_change : state_changes)
    {
        std::memset(&coords, 0, sizeof(coords));
        std::memset(&properties, 0, sizeof(properties));

        int contacts_in_event = 0;
        int action_index = 0;
        for (size_t i = 0, e = mir_touch_event_point_count(touch); i != e; ++i)
        {
            auto const action = mir_touch_event_action(touch, i);

            if (i == state_change.index)
                action_index = contacts_in_event;

            // before a touch up state change got processed it is treated as 'change', skipped otherwise
            // after a touch down state change got processed it is treated as 'change', skipped otherwise
            if (i == state_change.index
                || (i < state_change.index && action != mir_touch_action_up)
                || (i > state_change.index && action != mir_touch_action_down))
            {
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_X, mir_touch_event_axis_value(touch, i, mir_touch_axis_x));
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_Y, mir_touch_event_axis_value(touch, i, mir_touch_axis_y));
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, mir_touch_event_axis_value(touch, i, mir_touch_axis_touch_major));
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, mir_touch_event_axis_value(touch, i, mir_touch_axis_touch_minor));
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_SIZE, mir_touch_event_axis_value(touch, i, mir_touch_axis_size));
                coords[contacts_in_event].setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, mir_touch_event_axis_value(touch, i, mir_touch_axis_pressure));
                properties[contacts_in_event].toolType = mia::android_tool_type_from_mir(mir_touch_event_tooltype(touch, i));
                properties[contacts_in_event].id = mir_touch_event_id(touch, i);
                ++contacts_in_event;
            }
        }

        state_change.android_action |= (action_index << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

        ret = publisher.publishMotionEvent(seq, mir_input_event_get_device_id(input_event), AINPUT_SOURCE_TOUCHSCREEN,
                                           state_change.android_action, flags, edge_flags,
                                           mia::android_modifiers_from_mir(mir_touch_event_modifiers(touch)),
                                           button_state, x_offset, y_offset, x_precision, y_precision, event.motion.cookie,
                                           event_time, event_time, contacts_in_event, properties, coords);
    }

    return ret;
}

droidinput::status_t mia::InputSender::ActiveTransfer::send_pointer_event(uint32_t seq, MirEvent const& event)
{
    droidinput::PointerCoords pointer_coord;
    droidinput::PointerProperties pointer_properties;
    std::memset(&pointer_coord, 0, sizeof(pointer_coord));
    std::memset(&pointer_properties, 0, sizeof(pointer_properties));

    auto input_event = mir_event_get_input_event(&event);
    auto pointer = mir_input_event_get_pointer_event(input_event);
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_X, mir_pointer_event_axis_value(pointer, mir_pointer_axis_x));
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_Y, mir_pointer_event_axis_value(pointer, mir_pointer_axis_y));
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_HSCROLL, mir_pointer_event_axis_value(pointer, mir_pointer_axis_hscroll));
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_VSCROLL, mir_pointer_event_axis_value(pointer, mir_pointer_axis_vscroll));
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_RX, mir_pointer_event_axis_value(pointer, mir_pointer_axis_relative_x));
    pointer_coord.setAxisValue(AMOTION_EVENT_AXIS_RY, mir_pointer_event_axis_value(pointer, mir_pointer_axis_relative_y));
    pointer_properties.toolType = AMOTION_EVENT_TOOL_TYPE_MOUSE;
    pointer_properties.id = 0;

    std::chrono::nanoseconds const event_time{mir_input_event_get_event_time(input_event)};
    auto const x_offset = 0.0f;
    auto const y_offset = 0.0f;
    auto const x_precision = 0;
    auto const y_precision = 0;
    auto const flags = 0;
    auto const edge_flags = 0;
    return publisher.publishMotionEvent(
        seq, mir_input_event_get_device_id(input_event), AINPUT_SOURCE_MOUSE,
        mia::android_pointer_action_from_mir(mir_pointer_event_action(pointer), mir_pointer_event_buttons(pointer)),
        flags, edge_flags, mia::android_modifiers_from_mir(mir_pointer_event_modifiers(pointer)),
        mia::android_pointer_buttons_from_mir(mir_pointer_event_buttons(pointer)), x_offset, y_offset, x_precision,
        y_precision, event.motion.cookie, event_time, event_time, 1, &pointer_properties, &pointer_coord);
}


void mia::InputSender::ActiveTransfer::on_finish_signal()
{
    uint32_t sequence;
    bool handled;

    while(droidinput::OK == publisher.receiveFinishedSignal(&sequence, &handled));
}

bool mia::InputSender::ActiveTransfer::used_for_surface(input::Surface const* surface) const
{
    return this->surface == surface;
}
