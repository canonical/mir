/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/client/screencast_stream.h"
#include "mir/client_platform.h"
#include "mir/test/doubles/null_client_buffer.h"
#include "mir/test/doubles/mock_client_buffer_factory.h"
#include "mir/test/doubles/mock_protobuf_server.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir_test_framework/stub_client_platform_factory.h"
#include "mir/test/doubles/null_logger.h"
#include "mir/test/fake_shared.h"
#include "mir_toolkit/mir_client_library.h"

#include <exception>
#include <atomic>

namespace mp = mir::protobuf;
namespace ml = mir::logging;
namespace mg = mir::graphics;
namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mt::doubles;
using namespace testing;

namespace
{
MirBufferPackage a_buffer_package()
{
    MirBufferPackage bp;
    bp.fd_items = 1;
    bp.fd[0] = 16;
    bp.data_items = 2;
    bp.data[0] = 100;
    bp.data[1] = 234;
    bp.stride = 768;
    bp.width = 90;
    bp.height = 30;
    return bp;
}

std::atomic<int> unique_buffer_id{1};
void fill_protobuf_buffer_from_package(mp::Buffer* mb, MirBufferPackage const& buffer_package)
{
    mb->set_buffer_id(unique_buffer_id++);
    mb->set_fds_on_side_channel(buffer_package.fd_items);
    for (int i=0; i<buffer_package.data_items; i++)
        mb->add_data(buffer_package.data[i]);
    for (int i=0; i<buffer_package.fd_items; i++)
        mb->add_fd(buffer_package.fd[i]);
    mb->set_stride(buffer_package.stride);
    mb->set_width(buffer_package.width);
    mb->set_height(buffer_package.height);
}
    
struct ScreencastStream : Test
{
    ScreencastStream()
    {
        ON_CALL(mock_factory, create_buffer(_,_,_))
            .WillByDefault(Return(std::make_shared<mtd::NullClientBuffer>()));
    }

    mp::BufferStream a_protobuf_buffer_stream(MirPixelFormat format, MirBufferUsage usage, MirBufferPackage const& package)
    {
        mp::BufferStream protobuf_bs;
        mp::BufferStreamId bs_id;
        
        bs_id.set_value(1);
        *protobuf_bs.mutable_id() = bs_id;

        protobuf_bs.set_pixel_format(format);
        protobuf_bs.set_buffer_usage(usage);

        fill_protobuf_buffer_from_package(protobuf_bs.mutable_buffer(), package);
        return protobuf_bs;
    }

    testing::NiceMock<mtd::MockClientBufferFactory> mock_factory;
    mtd::StubClientBufferFactory stub_factory;

    testing::NiceMock<mtd::MockProtobufServer> mock_protobuf_server;

    MirPixelFormat const default_pixel_format = mir_pixel_format_argb_8888;
    MirBufferUsage const default_buffer_usage = mir_buffer_usage_hardware;

    MirBufferPackage buffer_package = a_buffer_package();
    geom::Size size{buffer_package.width, buffer_package.height};
    mp::BufferStream response = a_protobuf_buffer_stream(
        default_pixel_format, default_buffer_usage, buffer_package);
};
}

TEST_F(ScreencastStream, requests_screencast_buffer_when_next_buffer_called)
{
    EXPECT_CALL(mock_protobuf_server, screencast_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::ScreencastStream stream(
        nullptr, mock_protobuf_server, 
        std::make_shared<mir_test_framework::StubClientPlatform>(nullptr), response);
    auto wh = stream.next_buffer([]{});
    ASSERT_THAT(wh, NotNull());
    EXPECT_FALSE(wh->is_pending());
}

TEST_F(ScreencastStream, advances_current_buffer)
{
    int id0 = 33;
    int id1 = 34;

    EXPECT_CALL(mock_protobuf_server, screencast_buffer(_,_,_))
        .Times(2)
        .WillOnce(Invoke(
            [&](mp::ScreencastId const*, mp::Buffer* b, google::protobuf::Closure* c) {
                b->set_buffer_id(id0);
                c->Run();
            }))
        .WillOnce(Invoke(
            [&](mp::ScreencastId const*, mp::Buffer* b, google::protobuf::Closure* c) {
                b->set_buffer_id(id1);
                c->Run();
            }));

    mcl::ScreencastStream stream(
        nullptr, mock_protobuf_server, 
        std::make_shared<mir_test_framework::StubClientPlatform>(nullptr), response);

    auto wh = stream.next_buffer([]{});
    wh->wait_for_all();

    EXPECT_THAT(stream.get_current_buffer_id(), Eq(id0));

    wh = stream.next_buffer([]{});
    wh->wait_for_all();

    EXPECT_THAT(stream.get_current_buffer_id(), Eq(id1));
}

TEST_F(ScreencastStream, exception_does_not_leave_wait_handle_hanging)
{
    struct FailingBufferFactory : mcl::ClientBufferFactory
    {
        std::shared_ptr<mcl::ClientBuffer> create_buffer(
            std::shared_ptr<MirBufferPackage> const&, geom::Size, MirPixelFormat)
        {
            if (fail)
                throw std::runtime_error("monkey wrench");
            else
                return nullptr;
        }

        void start_failing()
        {
            fail = true;
        }

    private:
        bool fail = false;
    };

    struct BufferCreationFailingPlatform : mir_test_framework::StubClientPlatform
    {
        BufferCreationFailingPlatform(
            std::shared_ptr<mcl::ClientBufferFactory> const& factory, mcl::ClientContext* context) :
            StubClientPlatform(context),
            factory(factory)
        {
        }
        std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory()
        {
            return factory;
        }
        std::shared_ptr<mcl::ClientBufferFactory> const factory;
    };
    auto factory = std::make_shared<FailingBufferFactory>();
    auto platform = std::make_shared<BufferCreationFailingPlatform>(factory, nullptr);

    mcl::ScreencastStream stream(nullptr, mock_protobuf_server, platform, response);
    factory->start_failing();

    auto wh = stream.next_buffer([]{});
    wh->wait_for_all();
    EXPECT_FALSE(wh->is_pending());
}
