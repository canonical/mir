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


#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_STACK_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_STACK_H_

#include "mir/shell/surface_stack.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurfaceStack : public shell::SurfaceStack
{
    MOCK_METHOD1(raise, void(std::weak_ptr<scene::Surface> const&));
    MOCK_METHOD1(raise, void(scene::SurfaceSet const&));

    MOCK_METHOD2(add_surface, void(std::shared_ptr<scene::Surface> const&, input::InputReceptionMode new_mode));

    MOCK_METHOD1(remove_surface, void(std::weak_ptr<scene::Surface> const& surface));
    MOCK_CONST_METHOD1(surface_at, std::shared_ptr<scene::Surface>(geometry::Point));
    MOCK_METHOD2(swap_z_order, void(scene::SurfaceSet const&, scene::SurfaceSet const&));
    MOCK_METHOD1(send_to_back, void(scene::SurfaceSet const&));
    MOCK_CONST_METHOD2(is_above, bool(std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b));
};

}
}
}


#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_STACK_H_ */
