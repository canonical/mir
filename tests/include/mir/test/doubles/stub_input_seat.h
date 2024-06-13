/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_STUB_INPUT_SEAT_H
#define MIR_STUB_INPUT_SEAT_H

#include <mir/input/seat.h>
#include <mir/events/event_builders.h>

namespace mir
{
namespace test
{
namespace doubles
{
class StubInputSeat : public mir::input::Seat
{
public:
    void add_device(mir::input::Device const&) override {}
    void remove_device(mir::input::Device const&) override {}
    void dispatch_event(std::shared_ptr<MirEvent> const&) override {}

    auto create_device_state() -> mir::EventUPtr override
    {
        std::vector<mir::events::InputDeviceState> devices;
        MirPointerButtons buttons = 0;
        return mir::events::make_input_configure_event(
            std::chrono::system_clock::now().time_since_epoch(),
            buttons,
            mapper.modifiers(),
            0,
            0,
            std::move(devices)
        );
    }

    auto xkb_modifiers() const -> MirXkbModifiers override
    {
        return mapper.xkb_modifiers();
    }

    void set_key_state(mir::input::Device const&, std::vector<uint32_t> const&) override {}
    void set_pointer_state(mir::input::Device const&, MirPointerButtons) override {}
    void set_cursor_position(float, float) override {}
    void set_confinement_regions(mir::geometry::Rectangles const&) override {}
    void reset_confinement_regions() override {}

    auto bounding_rectangle() const -> mir::geometry::Rectangle override
    {
        return {};
    }

    auto output_info(uint32_t) const -> mir::input::OutputInfo override
    {
        return {};
    }

private:
    mir::input::receiver::XKBMapper mapper;
};
}
}
}

#endif //MIR_STUB_INPUT_SEAT_H
