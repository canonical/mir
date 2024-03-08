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

#ifndef MIR_TEST_DOUBLES_MOCK_SEAT_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_SEAT_REPORT_H_

#include "mir/input/seat_observer.h"
#include <gmock/gmock.h>

class MirEvent;

namespace mir
{
namespace test
{
namespace doubles
{

class MockSeatObserver : public input::SeatObserver
{
public:
    MOCK_METHOD(void, seat_add_device, (uint64_t /*id*/), (override));
    MOCK_METHOD(void, seat_remove_device, (uint64_t /*id*/), (override));
    MOCK_METHOD(void, seat_dispatch_event, (std::shared_ptr<MirEvent const> const& /*event*/), (override));
    MOCK_METHOD(void, seat_set_key_state, (uint64_t /*id*/, std::vector<uint32_t> const& /*scan_codes*/), (override));
    MOCK_METHOD(void, seat_set_pointer_state, (uint64_t /*id*/, unsigned /*buttons*/), (override));
    MOCK_METHOD(void, seat_set_cursor_position, (float /*cursor_x*/, float /*cursor_y*/), (override));
    MOCK_METHOD(void, seat_set_confinement_region_called, (geometry::Rectangles const& /*regions*/), (override));
    MOCK_METHOD(void, seat_reset_confinement_regions, (), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SEAT_REPORT_H_ */
