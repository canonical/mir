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

#include "mir/input/seat.h"
#include "mir_toolkit/events/event.h"

#include <functional>
#include <mutex>
#include <vector>
#include <memory>

namespace mir
{
namespace time
{
class Clock;
}
namespace input
{
class EventBuilder;
class InputEventTransformer : public Seat
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
        virtual bool transform_input_event(EventDispatcher const&, EventBuilder*, MirEvent const&) = 0;
    };

    InputEventTransformer(std::shared_ptr<Seat> const& seat, std::shared_ptr<time::Clock> const& clock);
    ~InputEventTransformer();

    bool transform(MirEvent const& event);

    virtual void append(std::weak_ptr<Transformer> const&);
    virtual void remove(std::shared_ptr<Transformer> const&);

    virtual void add_device(Device const& device) override;
    virtual void remove_device(Device const& device) override;
    virtual void dispatch_event(std::shared_ptr<MirEvent> const& event) override;
    virtual EventUPtr create_device_state() override;
    virtual auto xkb_modifiers() const -> MirXkbModifiers override;
    virtual void set_key_state(Device const& dev, std::vector<uint32_t> const& scan_codes) override;
    virtual void set_pointer_state(Device const& dev, MirPointerButtons buttons) override;
    virtual void set_cursor_position(float cursor_x, float cursor_y) override;
    virtual void set_confinement_regions(geometry::Rectangles const& regions) override;
    virtual void reset_confinement_regions() override;
    virtual geometry::Rectangle bounding_rectangle() const override;
    virtual input::OutputInfo output_info(uint32_t output_id) const override;

private:
    std::mutex mutex;
    std::vector<std::weak_ptr<Transformer>> input_transformers;

    MirInputDeviceId const id{-1};
    std::shared_ptr<Seat> const seat;
    std::shared_ptr<time::Clock> const clock;
    std::unique_ptr<EventBuilder> const builder;
    std::function<void(std::shared_ptr<MirEvent>)> const dispatcher;
};
}
}

#endif
