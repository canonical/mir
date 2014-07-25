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
     * Enable Project Butter input resampling:
     *  1. Only consume batches if a previous call started one. This way
     *     raw events are allowed time to be grouped and resampled (prediction)
     *     This effectively means we're scaling the flushing deadline with
     *     the raw event rate. And under busy conditions, batches are fully
     *     flushed to the client on every second call. That's the lowest
     *     latency possible while keeping the resampling code path active.
     *  2. Set frameTime=INT64_MAX. A positive value is required to enable
     *     resampling. But giving an honest frame time only results in all
     *     events being delivered to the client one frame late.
     *     So give a fake frame time that's always far enough ahead in the
     *     future that brand new raw events are instantly considered "old"
     *     enough to get processed and factored into the motion predicition.
     *
     * This is actually better than how Project Butter was designed to work.
     * Doing it this way yields much lower latency and has the benefit of not
     * having to talk to any graphics code about frame times.
     */

    if (input_consumer->consume(&event_factory,
                                input_consumer->hasPendingBatch(),
                                INT64_MAX, &event_sequence_id, &android_event)
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
        /*
         * A batch is pending and must be completed or else the client could
         * be starved of events. But don't hurry. A continuous motion gesture
         * will wake us up much sooner than 50ms. This timeout is only reached
         * in the case that motion has ended (fingers lifed).
         */
        std::chrono::milliseconds const motion_idle_timeout(50);
        if (timeout.count() < 0 || timeout > motion_idle_timeout)
            reduced_timeout = motion_idle_timeout;
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
