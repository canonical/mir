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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_

#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/input/input_sink.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInputSeat : input::Seat
{
    MOCK_METHOD(void, add_device, (input::Device const& device), ());
    MOCK_METHOD(void, remove_device, (input::Device const& device), ());
    MOCK_METHOD(void, dispatch_event, (std::shared_ptr<MirEvent> const& event), ());
    MOCK_METHOD(mir::EventUPtr, create_device_state, (), ());
    MOCK_METHOD(MirXkbModifiers, xkb_modifiers, (), (const));
    MOCK_METHOD(void, set_key_state, (input::Device const&, std::vector<uint32_t> const&), ());
    MOCK_METHOD(void, set_pointer_state, (input::Device const&, MirPointerButtons), ());
    MOCK_METHOD(void, set_cursor_position, (float, float), ());
    MOCK_METHOD(void, set_confinement_regions, (geometry::Rectangles const&), ());
    MOCK_METHOD(void, reset_confinement_regions, (), ());
    MOCK_METHOD(geometry::Rectangle, bounding_rectangle, (), (const));
    MOCK_METHOD(input::OutputInfo, output_info, (uint32_t), (const));
};
}
}
}

#endif
