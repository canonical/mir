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
    last_seq(0),
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
    uint32_t next_seq;

    if (last_seq)
    {
        input_consumer->sendFinishedSignal(last_seq, true);
        last_seq = 0;
    }

    /*
     * Simulate a frame counter that comes in phase with the present every
     * 16ms. Not sure how important getting real frame timing is...
     */
    nsecs_t last_frame_time = systemTime(SYSTEM_TIME_MONOTONIC);
    nsecs_t const one_frame = 16000000LL;
    last_frame_time -= (last_frame_time % one_frame);

    /*
     * Try to batch input events first, grouping multiple events inside the
     * duration of a frame so as to return the most current information when
     * it's needed. 
     * This also means the client wastes less time immediately reacting
     * to the oldest motion coordinates in the queue.
     */
    if (input_consumer->consume(&event_factory, false,
            last_frame_time, &next_seq, &android_event) == droidinput::OK
        ||
        input_consumer->consume(&event_factory, true,
            last_frame_time, &next_seq, &android_event) == droidinput::OK)
    {
        mia::Lexicon::translate(android_event, ev);

        map_key_event(xkb_mapper, ev);

        last_seq = next_seq;

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

    if(try_next_event(ev))
        return true;

    if (!input_consumer->hasDeferredEvent() &&
        !input_consumer->hasPendingBatch())
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
