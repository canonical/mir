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

#include "src/client/mir_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(MirSurfaceId, roundtrips_to_string)
{
    using namespace testing;

    MirSurfaceId id_one{"c43a798c-60d8-4ce3-89f8-3e42f4c5f7af"};

    MirSurfaceId id_two{id_one.as_string()};

    EXPECT_THAT(id_one, Eq(id_two));
}
