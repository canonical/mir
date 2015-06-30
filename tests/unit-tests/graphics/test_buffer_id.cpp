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
#include "mir/test/doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;

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
