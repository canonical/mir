/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
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
    MOCK_METHOD1(seat_add_device, void(uint64_t /*id*/));
    MOCK_METHOD1(seat_remove_device, void(uint64_t /*id*/));
    MOCK_METHOD1(seat_dispatch_event, void(std::shared_ptr<MirEvent const> const& /*event*/));
    MOCK_METHOD2(seat_get_rectangle_for, void(uint64_t /*id*/, geometry::Rectangle const& /*out_rect*/));
    MOCK_METHOD2(seat_set_key_state, void(uint64_t /*id*/, std::vector<uint32_t> const& /*scan_codes*/));
    MOCK_METHOD2(seat_set_pointer_state, void(uint64_t /*id*/, unsigned /*buttons*/));
    MOCK_METHOD2(seat_set_cursor_position, void(float /*cursor_x*/, float /*cursor_y*/));
    MOCK_METHOD1(seat_set_confinement_region_called, void(geometry::Rectangles const& /*regions*/));
    MOCK_METHOD0(seat_reset_confinement_regions, void());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SEAT_REPORT_H_ */
