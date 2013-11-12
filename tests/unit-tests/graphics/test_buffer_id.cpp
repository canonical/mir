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

#include "mir/graphics/buffer_id.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;

TEST(buffer_id, value_set )
{
    unsigned int id_as_int = 44;
    mg::BufferID id{id_as_int};
    EXPECT_EQ(id_as_int, id.as_uint32_t());
}

TEST(buffer_id, equality_testable)
{
    unsigned int id_as_int0 = 44;
    unsigned int id_as_int1 = 41;

    mg::BufferID id0{id_as_int0};
    mg::BufferID id1{id_as_int1};

    EXPECT_EQ(id0, id0);
    EXPECT_EQ(id1, id1);
    EXPECT_NE(id0, id1);
    EXPECT_NE(id1, id0);
}

TEST(buffer_id, less_than_testable)
{
    unsigned int id_as_int0 = 44;
    unsigned int id_as_int1 = 41;

    mg::BufferID id0{id_as_int0};
    mg::BufferID id1{id_as_int1};

    EXPECT_LT(id1, id0);
}

TEST(unique_generator, generate_unique)
{
    using mir::test::doubles::StubBuffer;
    int const ids = 542;
    std::vector<mg::BufferID> generated_ids;

    for (auto i=0; i < ids; i++)
        generated_ids.push_back(StubBuffer().id());

    while (!generated_ids.empty())
    {
        mg::BufferID test_id = generated_ids.back();

        generated_ids.pop_back();

        for (auto id : generated_ids)
        {
            EXPECT_NE(id, test_id);
        }
    }
}
