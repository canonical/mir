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

#include "mir/frontend/surface_stack.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockFrontendSurfaceStack : public frontend::SurfaceStack
{
    MOCK_METHOD((void), add_observer, (std::shared_ptr<scene::Observer> const& observer), (override));
    MOCK_METHOD((void), remove_observer, (std::weak_ptr<scene::Observer> const& observer), (override));
    MOCK_METHOD((scene::SurfaceList), stacking_order_of, (scene::SurfaceSet const& surfaces), (const));
    MOCK_METHOD((bool), screen_is_locked, (), (const, override));
};

}
}
}


#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_STACK_H_ */
