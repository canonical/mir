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

    auto& id_one = map.id_for_surface(surface);
    auto& id_two = map.id_for_surface(surface);

    EXPECT_THAT(id_one, Eq(std::ref(id_two)));
}

TEST(PersistentSurfaceStore, id_is_stable_under_aliasing)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto surface_alias = surface;

    auto& id_one = map.id_for_surface(surface);
    auto& id_two = map.id_for_surface(surface_alias);

    EXPECT_THAT(id_one, Eq(std::ref(id_two)));
}

TEST(PersistentSurfaceStore, can_lookup_surface_by_id)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto& id = map.id_for_surface(surface);

    auto looked_up_surface = map.surface_for_id(id);

    EXPECT_THAT(looked_up_surface, Eq(surface));
}

TEST(PersistentSurfaceStore, retrieves_correct_surface)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    auto surface_one = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto surface_two = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto& id_one = map.id_for_surface(surface_one);
    auto& id_two = map.id_for_surface(surface_two);

    auto looked_up_surface_one = map.surface_for_id(id_one);
    auto looked_up_surface_two = map.surface_for_id(id_two);

    EXPECT_THAT(looked_up_surface_one, Eq(surface_one));
    EXPECT_THAT(looked_up_surface_two, Eq(surface_two));
}

TEST(PersistentSurfaceStore, can_roundtrip_ids_to_strings)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    auto& id_one = map.id_for_surface(surface);

    auto buf = map.serialise_id(id_one);
    auto& id_two = map.deserialise_id(buf);

    EXPECT_THAT(id_one, Eq(std::ref(id_two)));
}

TEST(PersistentSurfaceStore, deserialising_wildly_incorrect_buffer_raises_exception)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    std::vector<uint8_t> buf(5, 'a');

    EXPECT_THROW(map.deserialise_id(buf), std::invalid_argument);
}

TEST(PersistentSurfaceStore, deserialising_invalid_buffer_raises_exception)
{
    using namespace testing;

    msh::DefaultPersistentSurfaceStore map;

    // This is the right size, but isn't a UUID because it lacks the XX-XX-XX structure
    std::vector<uint8_t> buf(36, 'a');

    EXPECT_THROW(map.deserialise_id(buf), std::invalid_argument);
}
