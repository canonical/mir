/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_TRACKER_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_TRACKER_H_

#include "src/server/frontend/surface_tracker.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockSurfaceTracker : public frontend::SurfaceTracker
{
    MOCK_METHOD2(track_buffer, bool(frontend::SurfaceId, graphics::Buffer*));
    MOCK_METHOD1(remove_surface, void(frontend::SurfaceId));
    MOCK_CONST_METHOD1(last_buffer, graphics::Buffer*(frontend::SurfaceId));
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_TRACKER_H_ */
