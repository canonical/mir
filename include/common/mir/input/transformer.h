/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_TRANSFORMER_H_
#define MIR_INPUT_TRANSFORMER_H_

#include <functional>
#include <memory>

class MirEvent;

namespace mir
{
namespace input
{
class EventBuilder;
/// Transformers are used early on in the input processing pipeline to process
/// events for various accessibilty methods. They can consume or pass events,
/// similar to filters. They additionally can emit events at a later time using
/// the passed `EventBuilder` and `EventDispatcher`.
///
/// Do note that despite the event dispatcher being a reference and the event
/// builder being a pointer, they're guaranteed to outlive the transformer.
///
/// Transformers need to be registered to an `InputEventTransformer` to receive
/// events. At the moment, the central instance of `InputEventTransformer` is
/// not accessible from miral. This class is only exposed for testing.
class Transformer
{
public:
    using EventDispatcher = std::function<void(std::shared_ptr<MirEvent>)>;

    virtual ~Transformer() = default;

    // Returning true means that the event has been successfully processed and
    // shouldn't be handled by later transformers, whether the transformer is
    // accumulating events for later dispatching or has immediately dispatched
    // an event is an implementation detail of the transformer
    virtual bool transform_input_event(EventDispatcher const&, EventBuilder*, MirEvent const&) = 0;
};
}
}

#endif // MIR_INPUT_INPUT_EVENT_TRANSFORMER_H_ 
