/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/frontend/application_manager.h"
#include "mir/surfaces/application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(add_surface, void(std::weak_ptr<ms::Surface> surface));
    MOCK_METHOD1(remove_surface, void(std::weak_ptr<ms::Surface> surface));
};

}

TEST(ApplicationManagerDeathTest, class_invariants_not_satisfied_triggers_assertion)
{
    EXPECT_EXIT(
        mir::frontend::ApplicationManager app(nullptr),
        ::testing::KilledBySignal(SIGABRT),
        ".*");
}

TEST(ApplicationManager, add_and_remove_surface)
{
    using namespace ::testing;
    
    MockApplicationSurfaceOrganiser organizer;
    mf::ApplicationManager app_manager(&organizer);

    EXPECT_CALL(organizer, add_surface(_));
    EXPECT_CALL(organizer, remove_surface(_));

    std::shared_ptr<ms::Surface> surface{app_manager.create_surface()};
    ASSERT_TRUE(surface.get() != nullptr);
    
    app_manager.destroy_surface(surface);
}

