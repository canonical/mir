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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/client/buffer_stream.h"

#include "mir_test_doubles/null_client_buffer.h"
#include "mir_test_doubles/mock_client_buffer_factory.h"
#include "mir_test_doubles/stub_client_buffer_factory.h"
#include "mir_test/fake_shared.h"

#include <mir_toolkit/mir_client_library.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mp = mir::protobuf;
namespace mcl = mir::client;

namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct MockProtobufServer : public mp::DisplayServer
{
    MOCK_METHOD4(screencast_buffer,
                 void(google::protobuf::RpcController* /*controller*/,
                      mp::BufferStreamId const* /*request*/,
                      mp::Buffer* /*response*/,
                      google::protobuf::Closure* /*done*/));
    MOCK_METHOD4(exchange_buffer,
                 void(google::protobuf::RpcController* /*controller*/,
                      mp::BufferRequest const* /*request*/,
                      mp::Buffer* /*response*/,
                      google::protobuf::Closure* /*done*/));
};

struct MirBufferStreamTest : public testing::Test
{
    mtd::MockClientBufferFactory mock_client_buffer_factory;
    MockProtobufServer mock_protobuf_server;

    MirPixelFormat const default_pixel_format = mir_pixel_format_argb_8888;
    MirBufferUsage const default_buffer_usage = mir_buffer_usage_hardware;
};

void fill_protobuf_buffer_stream_from_package(mp::BufferStream &protobuf_bs, MirBufferPackage const& buffer_package)
{
    // TODO
    (void) protobuf_bs;
    (void) buffer_package;
}
    
MirBufferPackage a_buffer_package()
{
    // TODO
    MirBufferPackage bp;
    return bp;
}

mp::BufferStream a_protobuf_buffer_stream(MirPixelFormat format, MirBufferUsage usage, MirBufferPackage const& package)
{
    mp::BufferStream protobuf_bs;
    protobuf_bs.set_pixel_format(format);
    protobuf_bs.set_buffer_usage(usage);
    fill_protobuf_buffer_stream_from_package(protobuf_bs, package);
    return protobuf_bs;
}

MATCHER_P(BufferPackageMatches, package, "")
{
    if (package.data_items != arg->data_items)
        return false;
    if (package.fd_items != arg->fd_items)
        return false;
    if (memcmp(package.data, arg->data, sizeof(package.data[0]) * package.data_items))
        return false;
    if (package.stride != arg->stride)
        return false;
    if (package.width != arg->width)
        return false;
    if (package.height != arg->height)
        return false;
    return true;
}

ACTION(RunProtobufClosure)
{
    arg3->Run();
}

void null_callback_func(mcl::BufferStream*, void*)
{
}

}

TEST_F(MirBufferStreamTest, uses_buffer_message_from_server)
{
    using namespace ::testing;

    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package);

    fill_protobuf_buffer_stream_from_package(protobuf_bs, buffer_package);

    EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(std::make_shared<mtd::NullClientBuffer>()));

    mcl::BufferStream bs(mock_protobuf_server, mcl::BufferStreamMode::Producer, mt::fake_shared(mock_client_buffer_factory), protobuf_bs);
}

TEST_F(MirBufferStreamTest, producer_streams_call_exchange_buffer_on_next_buffer)
{
    using namespace ::testing;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    // TODO: Improve matching
    EXPECT_CALL(mock_protobuf_server, exchange_buffer(_,_,_,_))
        .WillOnce(RunProtobufClosure());

    mcl::BufferStream bs(mock_protobuf_server, mcl::BufferStreamMode::Producer, std::make_shared<mtd::StubClientBufferFactory>(), protobuf_bs);
    
    bs.next_buffer(null_callback_func, nullptr);
}

TEST_F(MirBufferStreamTest, consumer_streams_call_screencast_buffer_on_next_buffer)
{
    using namespace ::testing;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    // TODO: Improve matching
    EXPECT_CALL(mock_protobuf_server, screencast_buffer(_,_,_,_))
        .WillOnce(RunProtobufClosure());

    mcl::BufferStream bs(mock_protobuf_server, mcl::BufferStreamMode::Consumer, std::make_shared<mtd::StubClientBufferFactory>(), protobuf_bs);
    
    bs.next_buffer(null_callback_func, nullptr);
}

// TODO: Test callback for next buffer
// TODO: Test for surface parameters?
// TODO: Test get current buffer
// TODO: Test GetEGLNativeWindow
// TODO: Test graphis region
