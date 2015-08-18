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

#include "mir/test/doubles/mock_surface.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

TEST(DefaultPersistentSurfaceStore, id_for_surface_is_idempotent)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto id_one = store.id_for_surface(surface);
    auto id_two = store.id_for_surface(surface);

    EXPECT_THAT(id_one, Eq(std::ref(id_two)));
}

TEST(DefaultPersistentSurfaceStore, id_is_stable_under_aliasing)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto surface_alias = surface;

    auto id_one = store.id_for_surface(surface);
    auto id_two = store.id_for_surface(surface_alias);

    EXPECT_THAT(id_one, Eq(std::ref(id_two)));
}

TEST(DefaultPersistentSurfaceStore, can_lookup_surface_by_id)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto id = store.id_for_surface(surface);

    auto looked_up_surface = store.surface_for_id(id);

    EXPECT_THAT(looked_up_surface, Eq(surface));
}

TEST(DefaultPersistentSurfaceStore, retrieves_correct_surface)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    auto surface_one = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto surface_two = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto id_one = store.id_for_surface(surface_one);
    auto id_two = store.id_for_surface(surface_two);

    auto looked_up_surface_one = store.surface_for_id(id_one);
    auto looked_up_surface_two = store.surface_for_id(id_two);

    EXPECT_THAT(looked_up_surface_one, Eq(surface_one));
    EXPECT_THAT(looked_up_surface_two, Eq(surface_two));
}

TEST(DefaultPersistentSurfaceStore, looking_up_destroyed_surface_returns_nullptr)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto id = store.id_for_surface(surface);

    surface.reset();

    auto looked_up_surface = store.surface_for_id(id);

    EXPECT_THAT(looked_up_surface, Eq(nullptr));
}

TEST(DefaultPersistentSurfaceStore, looking_up_nonexistent_surface_throws)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore store;

    msh::PersistentSurfaceStore::Id id;

    EXPECT_THROW(store.surface_for_id(id), std::out_of_range);
}
