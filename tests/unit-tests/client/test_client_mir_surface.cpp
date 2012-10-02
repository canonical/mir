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

#include "mir_client/private/client_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace geom = mir::geometry;

namespace mir
{
namespace test
{ 

struct MockBuffer : public mcl::ClientBuffer
{
    MockBuffer()
    {

    }

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_METHOD0(width, geom::Width());
    MOCK_METHOD0(height, geom::Height());
    MOCK_METHOD0(pixel_format, geom::PixelFormat());
};

}
}

namespace mt = mir::test;

struct MirClientSurfaceTest : public testing::Test
{
    void SetUp()
    {

    }

    protobuf::DisplayServer::Stub server;
    MirSurfaceParameters const params;
};


void empty_callback(MirSurface *, void*) {};
TEST_F(MirClientSurfaceTest, next_buffer_creates_on_first)
{
    auto surface = mcl::MirSurface( server, params, empty_callback, NULL);
 
}
