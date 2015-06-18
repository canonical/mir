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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
//namespace mtd = mir::test::doubles;

using Id = msh::PersistentSurfaceStore::Id;

TEST(PersistentSurfaceStoreId, deserialising_wildly_incorrect_buffer_raises_exception)
{
    EXPECT_THROW(Id{"bob"}, std::invalid_argument);
}

TEST(PersistentSurfaceStoreId, deserialising_invalid_buffer_raises_exception)
{
    // This is the right size, but isn't a UUID because it lacks the XX-XX-XX structure
    EXPECT_THROW(Id{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}, std::invalid_argument);
}

TEST(PersistentSurfaceStoreId, serialization_roundtrips_with_deserialization)
{
    using namespace testing;
    Id first_id;

    auto const buf = first_id.serialize_to_string();
    Id const second_id{buf};

    EXPECT_THAT(second_id, Eq(first_id));
}

TEST(PersistentSurfaceStoreId, ids_assigned_evaluate_equal)
{
    using namespace testing;

    Id const first_id;

    auto const second_id = first_id;

    EXPECT_THAT(second_id, Eq(first_id));
}

TEST(PersistentSurfaceStoreId, equal_ids_hash_equally)
{
    using namespace testing;

    auto const uuid_string = "0744caf3-c8d9-4483-a005-3375c1954287";

    Id const first_id{uuid_string};
    Id const second_id{uuid_string};

    EXPECT_THAT(std::hash<Id>()(second_id), Eq(std::hash<Id>()(first_id)));
}

TEST(PersistentSurfaceStoreId, can_assign_ids)
{
    using namespace testing;

    Id first_id;
    Id second_id;

    // Technically, there's a roughly 1-in-2^128 chance of a false fail here.
    EXPECT_THAT(second_id, Not(Eq(first_id)));

    second_id = first_id;

    EXPECT_THAT(second_id, Eq(first_id));
}
