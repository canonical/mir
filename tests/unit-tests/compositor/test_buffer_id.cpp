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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(buffer_id, value_set )
{
    int id_as_int = 44;
    mc::BufferID id{id_as_int};
    EXPECT_EQ(id_as_int, id.as_int32_t());
}

TEST(buffer_id, equality_testable)
{
    int id_as_int0 = 44; 
    int id_as_int1 = 41;

    mc::BufferID id0{id_as_int0};
    mc::BufferID id1{id_as_int1};

    EXPECT_EQ(id0, id0);
    EXPECT_EQ(id1, id1);
    EXPECT_NE(id0, id1);
    EXPECT_NE(id1, id0);
}
