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
    MOCK_METHOD(void, raise, (std::weak_ptr<scene::Surface> const&), (override));
    MOCK_METHOD(void, raise, (scene::SurfaceSet const&), (override));

    MOCK_METHOD(void, add_surface, (std::shared_ptr<scene::Surface> const&, input::InputReceptionMode new_mode), (override));

    MOCK_METHOD(void, remove_surface, (std::weak_ptr<scene::Surface> const& surface), (override));
    MOCK_METHOD(std::shared_ptr<scene::Surface>, surface_at, (geometry::Point), (const, override));
    MOCK_METHOD(void, swap_z_order, (scene::SurfaceSet const&, scene::SurfaceSet const&), (override));
    MOCK_METHOD(void, send_to_back, (scene::SurfaceSet const&), (override));
    MOCK_METHOD(bool, is_above, (std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b), (const, override));
};

}
}
}


#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_STACK_H_ */
