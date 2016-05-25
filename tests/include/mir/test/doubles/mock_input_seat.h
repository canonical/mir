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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SEAT_H_

#include "mir/input/seat.h"
#include "mir/input/device.h"
#include "mir/test/gmock_fixes.h"

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
    MOCK_METHOD1(dispatch_event, void(MirEvent& event));
    MOCK_METHOD1(get_rectangle_for, geometry::Rectangle(input::Device const& dev));
    MOCK_METHOD0(create_device_state, mir::EventUPtr());
};
}
}
}

#endif
