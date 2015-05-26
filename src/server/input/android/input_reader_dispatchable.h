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

#ifndef MIR_INPUT_ANDROID_INPUT_READER_DISPATCHABLE_H_
#define MIR_INPUT_ANDROID_INPUT_READER_DISPATCHABLE_H_

#include "mir/input/legacy_input_dispatchable.h"

namespace android
{
class EventHubInterface;
class InputReaderInterface;
}
namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{

struct InputReaderDispatchable : LegacyInputDispatchable
{
    InputReaderDispatchable(std::shared_ptr<droidinput::EventHubInterface> const& event_hub,
                            std::shared_ptr<droidinput::InputReaderInterface> const& reader);
    Fd watch_fd() const override;
    bool dispatch(mir::dispatch::FdEvents events) override;
    mir::dispatch::FdEvents relevant_events() const override;
    void start() override;

private:
    std::shared_ptr<droidinput::EventHubInterface> const event_hub;
    std::shared_ptr<droidinput::InputReaderInterface> const reader;
};

}
}
}

#endif
