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

#ifndef MIR_INPUT_INPUT_EVENT_TRANSFORMER_H_
#define MIR_INPUT_INPUT_EVENT_TRANSFORMER_H_

#include "mir/input/event_filter.h"
#include "mir/input/event_builder.h"
#include "mir_toolkit/events/event.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <vector>
#include <memory>

namespace mir
{
class MainLoop;
namespace input
{
class VirtualInputDevice;
class InputDeviceRegistry;
class InputSink;
class EventBuilder;
class InputEventTransformer : public EventFilter
{
public:
    using EventDispatcher = std::function<void(std::shared_ptr<MirEvent>)>;
    class Transformer
    {
    public:
        virtual ~Transformer() = default;

        // Returning true means that the event has been successfully processed and
        // shouldn't be handled by later transformers, whether the transformer is
        // accumulating events for later dispatching or has immediately dispatched
        // an event is an implementation detail of the transformer
        virtual bool transform_input_event(EventDispatcher const&, EventBuilder*,  MirEvent const&) = 0;
    };

    struct Registration
    {
        Registration(
            InputEventTransformer* event_transformer,
            std::shared_ptr<mir::input::InputEventTransformer::Transformer> transformer);

        ~Registration();

        Registration(Registration&& other);

    private:
        Registration();

        Registration(Registration const&) = delete;
        auto operator=(Registration const&) noexcept -> Registration& = delete;
        auto operator=(Registration&&) noexcept -> Registration& = delete;

        void swap(Registration& other) noexcept;

        std::function<void()> unregister;
    };

    InputEventTransformer(std::shared_ptr<InputDeviceRegistry> const&, std::shared_ptr<MainLoop> const&);
    ~InputEventTransformer();

    bool handle(MirEvent const&) override;

    /// Appends the transformer to a list. It is assumed that only the only
    /// owners of this transformer are the managing object, and the
    /// `InputEventTransformer` that it's registered to.
    ///
    /// Duplicate appends will throw.
    [[nodiscard]] auto append(std::shared_ptr<Transformer> const&) -> Registration;

private:
    void remove(std::shared_ptr<Transformer> const&);

    std::mutex mutex;
    std::vector<std::shared_ptr<Transformer>> input_transformers;

    std::shared_ptr<input::VirtualInputDevice> const virtual_pointer;
    std::shared_ptr<input::InputDeviceRegistry> const input_device_registry;
    std::shared_ptr<MainLoop> const main_loop;
};
}
}

#endif
