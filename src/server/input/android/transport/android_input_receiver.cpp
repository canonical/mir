/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/input/android_input_receiver.h"
#include "android_input_lexicon.h"
#include "../android_input_constants.h"

#include <androidfw/InputTransport.h>

#include <poll.h>

namespace miat = mir::input::android::transport;

miat::InputReceiver::InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel)
  : input_channel(input_channel),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel))
{
}

miat::InputReceiver::InputReceiver(int fd)
  : input_channel(new droidinput::InputChannel(droidinput::String8("TODO: Name"), fd)),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel))
{
}

miat::InputReceiver::~InputReceiver()
{
}

int miat::InputReceiver::get_fd() const
{
    return input_channel->getFd();
}

bool miat::InputReceiver::next_event(MirEvent &ev)
{
    droidinput::InputEvent *android_event;
    uint32_t event_sequence_id;
    bool handled_event = false;
    
    // Non blocking
    droidinput::status_t status;
    while((status = input_consumer->consume(&event_factory, consume_batches,
                                          default_frame_time, &event_sequence_id, &android_event)) != droidinput::WOULD_BLOCK)
        {
            // todo: Racarr ~ Lol
            miat::Lexicon::translate(android_event, ev);
            input_consumer->sendFinishedSignal(event_sequence_id, true);
            handled_event = true;
        }


    // So far once we have sent an event to the client there is no chance for redispatch
    // so the client handles all events.
    
    return handled_event;
}

bool miat::InputReceiver::poll(std::chrono::milliseconds timeout)
{
    struct pollfd pfd;
    
    pfd.fd = get_fd();
    pfd.events = POLLIN;
    
    auto status = ::poll(&pfd, 1, timeout.count());
    
    if (status > 0)
        return true;
    if (status == 0)
        return false;
    
    // TODO: What to do in case of error? ~racarr
    return false;
}
