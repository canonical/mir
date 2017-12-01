/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_

#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/input/input_sink.h"
#include "mir/test/gmock_fixes.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInputSeat : input::Seat
{
    MOCK_METHOD1(add_device, void(input::Device const& device));
    MOCK_METHOD1(remove_device, void(input::Device const& device));
    MOCK_METHOD1(dispatch_event, void(std::shared_ptr<MirEvent> const& event));
    MOCK_METHOD0(create_device_state, mir::EventUPtr());
    MOCK_METHOD2(set_key_state, void(input::Device const&, std::vector<uint32_t> const&));
    MOCK_METHOD2(set_pointer_state, void (input::Device const&, MirPointerButtons));
    MOCK_METHOD2(set_cursor_position, void (float, float));
    MOCK_METHOD1(set_confinement_regions, void(geometry::Rectangles const&));
    MOCK_METHOD0(reset_confinement_regions, void());
    MOCK_CONST_METHOD0(bounding_rectangle, geometry::Rectangle());
    MOCK_CONST_METHOD1(output_info, input::OutputInfo(uint32_t));
};
}
}
}

#endif
