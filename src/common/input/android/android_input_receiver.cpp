/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_receiver.h"

#include "mir/input/xkb_mapper.h"
#include "mir/input/input_receiver_report.h"
#include "mir/input/android/android_input_lexicon.h"

#include <androidfw/InputTransport.h>
#include <utils/Looper.h>

namespace mircv = mir::input::receiver;
namespace mircva = mircv::android;

namespace mia = mir::input::android;

mircva::InputReceiver::InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel,
                                     std::shared_ptr<mircv::InputReceiverReport> const& report,
                                     AndroidClock clock)
  : input_channel(input_channel),
    report(report),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mircv::XKBMapper>()),
    android_clock(clock)
{
}

mircva::InputReceiver::InputReceiver(int fd,
                                     std::shared_ptr<mircv::InputReceiverReport> const& report,
                                     AndroidClock clock)
  : input_channel(new droidinput::InputChannel(droidinput::String8(""), fd)),
    report(report),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mircv::XKBMapper>()),
    android_clock(clock)
{
}

mircva::InputReceiver::~InputReceiver()
{
}

int mircva::InputReceiver::fd() const
{
    return input_channel->getFd();
}

namespace
{

static void map_key_event(std::shared_ptr<mircv::XKBMapper> const& xkb_mapper, MirEvent &ev)
{
    // TODO: As XKBMapper is used to track modifier state we need to use a seperate instance
    // of XKBMapper per device id (or modify XKBMapper semantics)
    if (ev.type != mir_event_type_key)
        return;

    xkb_mapper->update_state_and_map_event(ev.key);
}

}

bool mircva::InputReceiver::try_next_event(MirEvent &ev)
{
    droidinput::InputEvent *android_event;
    uint32_t event_sequence_id;

    /*
     * Enable "Project Butter" input resampling in InputConsumer::consume():
     *   consumeBatches = true, so as to ensure the "cooked" event rate that
     *      clients experience is at least the minimum of event_rate_hz
     *      and the raw device event rate.
     *   frame_time = A regular interval. This provides a virtual frame
     *      interval during which InputConsumer will collect raw events,
     *      resample them and emit a "cooked" event back to us at roughly every
     *      60th of a second. "cooked" events are both smoothed and
     *      extrapolated/predicted into the future (for tool=finger) giving the
     *      appearance of lower latency. Getting a real frame time from the
     *      graphics logic (which is messy) does not appear to be necessary to
     *      gain significant benefit.
     *
     * Note event_rate_hz is only 55Hz. This allows rendering to catch up and
     * overtake the event rate every ~12th frame (200ms) on a 60Hz display.
     * Thus on every 12th+1 frame, there will be zero buffer lag in responding
     * to the cooked input event we have given the client.
     * This phase control is useful as it eliminates the one frame of lag you
     * would otherwise never catch up to if the event rate was exactly the same
     * as the display refresh rate.
     */

    nsecs_t const now = android_clock(SYSTEM_TIME_MONOTONIC);
    int const event_rate_hz = 55;
    nsecs_t const one_frame = 1000000000ULL / event_rate_hz;
    nsecs_t frame_time = (now / one_frame) * one_frame;

    if (input_consumer->consume(&event_factory, true, frame_time,
                                &event_sequence_id, &android_event)
        == droidinput::OK)
    {
        mia::Lexicon::translate(android_event, ev);

        map_key_event(xkb_mapper, ev);

        input_consumer->sendFinishedSignal(event_sequence_id, true);

        report->received_event(ev);

        return true;
    }
   return false;
}

// TODO: We use a droidinput::Looper here for polling functionality but it might be nice to integrate
// with the existing client io_service ~racarr ~tvoss
bool mircva::InputReceiver::next_event(std::chrono::milliseconds const& timeout, MirEvent &ev)
{
    if (!fd_added)
    {
        // TODO: Why will this fail from the constructor? ~racarr
        looper->addFd(fd(), fd(), ALOOPER_EVENT_INPUT, nullptr, nullptr);
        fd_added = true;
    }

    auto reduced_timeout = timeout;
    if (input_consumer->hasDeferredEvent())
    {
        // consume() didn't finish last time. Retry it immediately.
        reduced_timeout = std::chrono::milliseconds::zero();
    }
    else if (input_consumer->hasPendingBatch())
    {
        // When in constant motion we will usually "hasPendingBatch".
        // But the batch won't get flushed until the next frame interval,
        // so be sure to use a non-zero sleep time to avoid spinning the CPU
        // for the whole interval...
        reduced_timeout = std::chrono::milliseconds(1);
    }

    auto result = looper->pollOnce(reduced_timeout.count());
    if (result == ALOOPER_POLL_WAKE)
        return false;
    if (result == ALOOPER_POLL_ERROR) // TODO: Exception?
       return false;

    return try_next_event(ev);
}

void mircva::InputReceiver::wake()
{
    looper->wake();
}
