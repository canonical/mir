/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@gmail.com>
 */

#include "input_reader_dispatchable.h"
#include <EventHub.h>
#include <InputReader.h>

namespace md = mir::dispatch;
namespace mia = mir::input::android;

mia::InputReaderDispatchable::InputReaderDispatchable(
    std::shared_ptr<droidinput::EventHubInterface> const& event_hub,
    std::shared_ptr<droidinput::InputReaderInterface> const& reader)
    : event_hub(event_hub), reader(reader)
{}

mir::Fd mia::InputReaderDispatchable::watch_fd() const
{
    return event_hub->fd();
}

bool mia::InputReaderDispatchable::dispatch(md::FdEvents events)
{
    if (events & md::FdEvent::error)
        return false;
    if (events & md::FdEvent::readable)
        reader->loopOnce();
    return true;
}

md::FdEvents mia::InputReaderDispatchable::relevant_events() const
{
    return md::FdEvent::readable;
}

void mia::InputReaderDispatchable::start()
{
    event_hub->flush();
}
