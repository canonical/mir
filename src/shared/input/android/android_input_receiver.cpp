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
#include "mir/input/android/android_input_lexicon.h"

#include <androidfw/InputTransport.h>
#include <utils/Looper.h>

namespace mcli = mir::client::input;
namespace mclia = mcli::android;

namespace mia = mir::input::android;

mclia::InputReceiver::InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel)
  : input_channel(input_channel),
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mcli::XKBMapper>())
{
}

mclia::InputReceiver::InputReceiver(int fd)
  : input_channel(new droidinput::InputChannel(droidinput::String8(""), fd)), 
    input_consumer(std::make_shared<droidinput::InputConsumer>(input_channel)),
    looper(new droidinput::Looper(true)),
    fd_added(false),
    xkb_mapper(std::make_shared<mcli::XKBMapper>())
{
}

mclia::InputReceiver::~InputReceiver()
{
}

int mclia::InputReceiver::fd() const
{
    return input_channel->getFd();
}

namespace
{

static void map_key_event(std::shared_ptr<mcli::XKBMapper> const& xkb_mapper, MirEvent &ev)
{
    // TODO: As XKBMapper is used to track modifier state we need to use a seperate instance
    // of XKBMapper per device id (or modify XKBMapper semantics)
    if (ev.type != mir_event_type_key)
        return;
    
    if (ev.key.action == mir_key_action_up)
        ev.key.key_code = xkb_mapper->release_and_map_key(ev.key.scan_code);
    else 
        ev.key.key_code = xkb_mapper->press_and_map_key(ev.key.scan_code);
        
}

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
        mia::Lexicon::translate(android_event, ev);

        map_key_event(xkb_mapper, ev);

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
