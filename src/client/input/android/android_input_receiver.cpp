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
#include "mir/events/event_private.h"

#include <boost/throw_exception.hpp>

#include <androidfw/InputTransport.h>

#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <system_error>
#include <cstdlib>

namespace mircv = mir::input::receiver;
namespace mircva = mircv::android;
namespace md = mir::dispatch;

namespace mia = mir::input::android;

namespace
{
int valid_fd_or_system_error(int fd, char const* message)
{
    if (fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 message}));
    return fd;
}
}

mircva::InputReceiver::InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel,
                                     std::shared_ptr<mircv::XKBMapper> const& keymapper,
                                     std::function<void(MirEvent*)> const& event_handling_callback,
                                     std::shared_ptr<mircv::InputReceiverReport> const& report)
  : wake_fd{valid_fd_or_system_error(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK),
        "Failed to create IO wakeup notifier")},
    input_channel(input_channel),
    handler{event_handling_callback},
    xkb_mapper(keymapper),
    report(report),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel))
{
    dispatcher.add_watch(wake_fd,
                         [this]() { woke(); });
    dispatcher.add_watch(mir::Fd{mir::IntOwnedFd{input_channel->getFd()}},
                         [this]() { woke(); });
}

mircva::InputReceiver::InputReceiver(int fd,
                                     std::shared_ptr<mircv::XKBMapper> const& keymapper,
                                     std::function<void(MirEvent*)> const& event_handling_callback,
                                     std::shared_ptr<mircv::InputReceiverReport> const& report)
    : InputReceiver(new droidinput::InputChannel(droidinput::String8(""), fd),
                    keymapper,
                    event_handling_callback,
                    report)
{
}

mircva::InputReceiver::~InputReceiver()
{
}

mir::Fd mircva::InputReceiver::watch_fd() const
{
    return dispatcher.watch_fd();
}

bool mircva::InputReceiver::dispatch(md::FdEvents events)
{
    return dispatcher.dispatch(events);
}

md::FdEvents mircva::InputReceiver::relevant_events() const
{
    return dispatcher.relevant_events();
}

namespace
{

static void map_key_event(std::shared_ptr<mircv::XKBMapper> const& xkb_mapper, MirEvent &ev)
{
    // TODO: As XKBMapper is used to track modifier state we need to use a seperate instance
    // of XKBMapper per device id (or modify XKBMapper semantics)
    if (mir_event_get_type(&ev) != mir_event_type_input)
        return;

    xkb_mapper->map_event(ev);
}

}

void mircva::InputReceiver::woke()
{
    uint64_t dummy;

    /*
     * We never care about the cause of the wakeup. Either real input or a
     * wake(). Either way the only requirement is that we woke() after each
     * wake() or real input channel event.
     */
    if (read(wake_fd, &dummy, sizeof(dummy)) != sizeof(dummy) &&
        errno != EAGAIN &&
        errno != EINTR)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to consume notification"}));
    }

    droidinput::InputEvent *android_event;
    uint32_t event_sequence_id;

    auto const frame_time = std::chrono::nanoseconds(-1);
    auto result = input_consumer->consume(&event_factory,
                                          true,
                                          frame_time,
                                          &event_sequence_id,
                                          &android_event);
    if (result == droidinput::OK)
    {
        auto ev = mia::Lexicon::translate(android_event);
        map_key_event(xkb_mapper, *ev);
        report->received_event(*ev);
        handler(ev.get());

        // TODO: It would be handy in future if handler returned a bool
        //       indicating whether the event was used so that if not it might
        //       get passed on to someone else - passed into here:
        input_consumer->sendFinishedSignal(event_sequence_id, true);
    }
    if (input_consumer->hasDeferredEvent())
    {
        // input_consumer->consume() can read an event from the fd and find that the event cannot
        // be added to the current batch.
        //
        // In this case, it emits the current batch and leaves the new event pending.
        // This means we have an event we need to dispatch, but as it has already been read from
        // the fd we cannot rely on being woken by the fd being readable.
        //
        // So, we ensure we'll appear dispatchable by pushing an event to the wakeup pipe.
        wake();
    }
}

void mircva::InputReceiver::wake()
{
    uint64_t one{1};
    if (write(wake_fd, &one, sizeof(one)) != sizeof(one))
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to notify IO loop"}));
    }
}
