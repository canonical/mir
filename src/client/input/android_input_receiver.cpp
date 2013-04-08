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

#include "android_input_receiver.h"
#include "mir/input/android/android_input_lexicon.h"

#include <androidfw/InputTransport.h>
#include <utils/Looper.h>

namespace mclia = mir::client::input::android;
namespace miat = mir::input::android::transport;

mclia::InputReceiver::InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel)
  : input_channel(input_channel),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false)
{
}

mclia::InputReceiver::InputReceiver(int fd)
  : input_channel(new droidinput::InputChannel(droidinput::String8(""), fd)), 
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false)
{
}

mclia::InputReceiver::~InputReceiver()
{
}

int mclia::InputReceiver::fd() const
{
    return input_channel->getFd();
}

// TODO: We use a droidinput::Looper here for polling functionality but it might be nice to integrate
// with the existing client io_service ~racarr ~tvoss
bool mclia::InputReceiver::next_event(std::chrono::milliseconds const& timeout, MirEvent &ev)
{
    droidinput::InputEvent *android_event;
    uint32_t event_sequence_id;
    bool handled_event = false;

    if (!fd_added)
    {
        // TODO: Why will this fail from the constructor? ~racarr
        looper->addFd(fd(), fd(), ALOOPER_EVENT_INPUT, nullptr, nullptr);
        fd_added = true;
    }
    
    auto result = looper->pollOnce(timeout.count());
    if (result == ALOOPER_POLL_WAKE)
        return false;
    else if (result == ALOOPER_POLL_ERROR) // TODO: Exception?
        return false;

    if(input_consumer->consume(&event_factory, true,
        -1, &event_sequence_id, &android_event) != droidinput::WOULD_BLOCK)
    {
        miat::Lexicon::translate(android_event, ev);
        input_consumer->sendFinishedSignal(event_sequence_id, true);
        handled_event = true;
    }

    // So far once we have sent an event to the client there is no chance for redispatch
    // so the client handles all events.
    
    return handled_event;
}

void mclia::InputReceiver::wake()
{
    looper->wake();
}
