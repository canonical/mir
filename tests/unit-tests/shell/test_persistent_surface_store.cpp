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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shell/persistent_surface_store.h"

#include "mir/scene/surface.h"

#include "src/server/shell/default_persistent_surface_store.h"

#include "mir_test_doubles/mock_surface.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

TEST(PersistentSurfaceStore, id_for_surface_is_idempotent)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto id_one = map.id_for_surface(surface);
    auto id_two = map.id_for_surface(surface);

    EXPECT_TRUE(*id_one == *id_two);
}
