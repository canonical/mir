/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */


#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_COORDINATOR_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_COORDINATOR_H_

#include "mir/scene/surface_coordinator.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurfaceCoordinator : public scene::SurfaceCoordinator
{
    MOCK_METHOD1(raise, void(std::weak_ptr<scene::Surface> const&));

    MOCK_METHOD2(add_surface, std::shared_ptr<scene::Surface>(
        scene::SurfaceCreationParameters const& params,
        scene::Session* session));

    MOCK_METHOD1(remove_surface, void(std::weak_ptr<scene::Surface> const& surface));
};

}
}
}


#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_COORDINATOR_H_ */
