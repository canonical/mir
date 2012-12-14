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

/* since we're testing functions that belong to a class that shouldn't have 
   a constructor, we test via a test class */

#include "mir/compositor/basic_buffer.h"

class TestClassBuffer : public mc::BasicBuffer
{
    StubBuffer(mc::BufferID id)
     : BasicBuffer(id)
    {
    }

    geom::Size size() const
    {
    }

    geom::Stride stride() const
    {
    }

    geom::PixelFormat pixel_format() const
    {
    }

    void bind_to_texture()
    {
    }

    std::shared_ptr<BufferIPCPackage> get_ipc_package() const
    {
    }
};

class GraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

TEST_F(GraphicBufferBasic, id_generated)
{
    mc::BufferID id{4};

    TestClassBuffer buffer(id); 
    auto test_id = buffer.id();

    EXPECT_EQ(test_id, id);
}
