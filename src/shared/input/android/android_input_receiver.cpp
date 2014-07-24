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
                                     std::shared_ptr<mircv::InputReceiverReport> const& report)
  : input_channel(input_channel),
    report(report),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mircv::XKBMapper>())
{
}

mircva::InputReceiver::InputReceiver(int fd,
                                     std::shared_ptr<mircv::InputReceiverReport> const& report)
  : input_channel(new droidinput::InputChannel(droidinput::String8(""), fd)),
    report(report),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mircv::XKBMapper>())
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
     * InputConsumer will only immediately consume events from frame_time-5ms.
     * With an honest frame_time that would add one frame of lag. With a
     * dishonest frame_time of "now" that would still mean lag is still always
     * at least 5ms. But we can do better than that -- provide a time in the
     * future so that the batch is never split. This way we can control
     * event deferral and grouping, resulting in sub-5ms lag.
     * This is preferable to providing a frame time of -1, as that is similar
     * but disables motion predicition and smoothing (which we want).
     */
    nsecs_t const one_millisec = 1000000LL;
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    nsecs_t future_frame_time = now + 10 * one_millisec;

    auto status = input_consumer->consume(&event_factory,
                                          input_consumer->hasPendingBatch(),
                                          future_frame_time,
                                          &event_sequence_id, &android_event);

    if (status == droidinput::OK)
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
bool mircva::InputReceiver::next_event(std::chrono::milliseconds const& max_timeout, MirEvent &ev)
{
    if (!fd_added)
    {
        // TODO: Why will this fail from the constructor? ~racarr
        looper->addFd(fd(), fd(), ALOOPER_EVENT_INPUT, nullptr, nullptr);
        fd_added = true;
    }

    auto timeout = max_timeout;
    if (input_consumer->hasDeferredEvent())
    {
        // consume() didn't finish last time. Retry it immediately.
        timeout = std::chrono::milliseconds::zero();
    }
    else if (input_consumer->hasPendingBatch())
    {
        /*
         * Ensure any pending batch is fully consumed within 32ms (~2 frames).
         * This actually reduces lag instead of increasing it, because it
         * allows the batch to accumulate more samples, which significantly
         * improves event currency and prediction in future frames when you're
         * dragging something for example.
         */
        timeout = std::chrono::milliseconds(32);
    }

    // Note timeout may be negative (infinity)
    if (timeout != std::chrono::milliseconds::zero())
    {
        auto result = looper->pollOnce(timeout.count());
        if (result == ALOOPER_POLL_WAKE)
            return false;
        if (result == ALOOPER_POLL_ERROR) // TODO: Exception?
            return false;
    }

    return try_next_event(ev);
}

void mircva::InputReceiver::wake()
{
    looper->wake();
}
