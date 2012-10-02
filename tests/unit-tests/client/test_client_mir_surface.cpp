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

#include "mir_protobuf.pb.h"
#include "mir_client/mir_client_library.h"
#include "mir_client/mir_logger.h"
#include "mir_client/private/client_buffer.h"
#include "mir_client/private/client_buffer_factory.h"
#include "mir_client/private/mir_rpc_channel.h"
#include "mir_client/private/mir_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
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

struct MockClientFactory : public mcl::ClientBufferFactory
{
    MOCK_METHOD1(create_buffer_from_ipc_message, std::shared_ptr<mcl::ClientBuffer>(const mcl::MirBufferPackage&));
};

}
}

namespace mt = mir::test;

struct MirClientSurfaceTest : public testing::Test
{
    void SetUp()
    {
        params = MirSurfaceParameters{"test", 33, 45, mir_pixel_format_rgba_8888};

        channel = std::make_shared<mcl::MirRpcChannel>("./test_file", logger); 
        server = std::make_shared<mp::DisplayServer::Stub>(channel.get());
    }

    std::shared_ptr<mp::DisplayServer::Stub> server;
    std::shared_ptr<mcl::MirRpcChannel> channel;
    mcl::ConsoleLogger logger;

    MirSurfaceParameters params;
    mt::MockClientFactory mock_factory;
};


void empty_callback(MirSurface*, void*) {}
TEST_F(MirClientSurfaceTest, next_buffer_creates_on_first)
{
    using namespace testing;

    auto surface = mcl::MirSurface( server, mock_factory, params, empty_callback, NULL);

    EXPECT_CALL(mock_factory, create_buffer_from_ipc_message(_));
 
}
