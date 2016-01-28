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
#include "src/client/perf_report.h"
#include "src/client/rpc/mir_display_server.h"

#include "mir/client_platform.h"

#include "mir/test/doubles/null_client_buffer.h"
#include "mir/test/doubles/mock_client_buffer_factory.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/test/doubles/null_logger.h"
#include "mir/test/fake_shared.h"

#include "mir_toolkit/mir_client_library.h"

#include <future>
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

ACTION(RunProtobufClosure)
{
    arg2->Run();
}

ACTION_P(SetResponseError, message)
{
    arg1->set_error(message);
}

struct MockProtobufServer : public mclr::DisplayServer
{
    MockProtobufServer() : mclr::DisplayServer(nullptr)
    {
        ON_CALL(*this, configure_buffer_stream(_,_,_))
            .WillByDefault(RunProtobufClosure());
        ON_CALL(*this, submit_buffer(_,_,_))
            .WillByDefault(RunProtobufClosure());
        ON_CALL(*this, allocate_buffers(_,_,_))
            .WillByDefault(DoAll(InvokeWithoutArgs([this]{ alloc_count++; }), RunProtobufClosure()));
        ON_CALL(*this, release_buffers(_,_,_))
            .WillByDefault(RunProtobufClosure());
    }

    MOCK_METHOD3(configure_buffer_stream, void(
        mir::protobuf::StreamConfiguration const*, 
        mp::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(screencast_buffer, void(
        mp::ScreencastId const* /*request*/,
        mp::Buffer* /*response*/,
        google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(allocate_buffers, void(
        mir::protobuf::BufferAllocation const*,
        mir::protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(release_buffers, void(
        mir::protobuf::BufferRelease const*,
        mir::protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(submit_buffer, void(
        mp::BufferRequest const* /*request*/,
        mp::Void* /*response*/,
        google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(exchange_buffer, void(
        mp::BufferRequest const* /*request*/,
        mp::Buffer* /*response*/,
        google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(create_buffer_stream, void(
        mp::BufferStreamParameters const* /*request*/,
        mp::BufferStream* /*response*/,
        google::protobuf::Closure* /*done*/));
    unsigned int alloc_count{0};
};

struct StubClientPlatform : public mcl::ClientPlatform
{
    StubClientPlatform(
        std::shared_ptr<mcl::ClientBufferFactory> const& bf)
        : buffer_factory(bf)
    {
    }
    MirPlatformType platform_type() const override
    {
        return MirPlatformType();
    }
    void populate(MirPlatformPackage& /* package */) const override
    {
    }
    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mcl::EGLNativeSurface * /* surface */) override
    {
        return std::make_shared<EGLNativeWindowType>(egl_native_window);
    }
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display() override
    {
        return nullptr;
    }
    MirNativeBuffer* convert_native_buffer(mg::NativeBuffer*) const override
    {
        return nullptr;
    }

    std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory() override
    {
        return buffer_factory;
    }
    MirPlatformMessage* platform_operation(MirPlatformMessage const* /* request */)
    {
        return nullptr;
    }
    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override
    {
        return mir_pixel_format_invalid;
    }
    static EGLNativeWindowType egl_native_window;
    std::shared_ptr<mcl::ClientBufferFactory> const buffer_factory;
};

struct MockPerfReport : public mcl::PerfReport
{
    MOCK_METHOD1(name_surface, void(char const*));
    MOCK_METHOD1(begin_frame, void(int));
    MOCK_METHOD1(end_frame, void(int));
};

struct MockClientBuffer : public mtd::NullClientBuffer
{
    MockClientBuffer(geom::Size size) :
        mtd::NullClientBuffer(size)
    {
    }
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
};

EGLNativeWindowType StubClientPlatform::egl_native_window{
    reinterpret_cast<EGLNativeWindowType>(&StubClientPlatform::egl_native_window)};

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

 // Just ensure we have a unique ID in order to not confuse the buffer depository caching logic...
std::atomic<int> unique_buffer_id{1};

void fill_protobuf_buffer_from_package(mp::Buffer* mb, MirBufferPackage const& buffer_package)
{
    mb->set_buffer_id(unique_buffer_id++);

    /* assemble buffers */
    mb->set_fds_on_side_channel(buffer_package.fd_items);
    for (int i=0; i<buffer_package.data_items; i++)
    {
        mb->add_data(buffer_package.data[i]);
    }
    for (int i=0; i<buffer_package.fd_items; i++)
    {
        mb->add_fd(buffer_package.fd[i]);
    }
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
    void service_requests_for(mcl::ClientBufferStream& bs, unsigned int count)
    {
        for(auto i = 0u; i < count; i++)
        {
            mp::Buffer buffer;
            fill_protobuf_buffer_from_package(&buffer, a_buffer_package());
            buffer.set_width(size.width.as_int());
            buffer.set_height(size.height.as_int());
            bs.buffer_available(buffer);
        }
    }

    std::shared_ptr<MirWaitHandle> wait_handle;
    testing::NiceMock<mtd::MockClientBufferFactory> mock_factory;
    mtd::StubClientBufferFactory stub_factory;

    testing::NiceMock<MockProtobufServer> mock_protobuf_server;

    MirPixelFormat const default_pixel_format = mir_pixel_format_argb_8888;
    MirBufferUsage const default_buffer_usage = mir_buffer_usage_hardware;

    std::shared_ptr<mcl::PerfReport> const perf_report = std::make_shared<mcl::NullPerfReport>();

    MirBufferPackage buffer_package = a_buffer_package();
    geom::Size size{buffer_package.width, buffer_package.height};
    mp::BufferStream response = a_protobuf_buffer_stream(
        default_pixel_format, default_buffer_usage, buffer_package);
};
}

TEST_F(ScreencastStream, consumer_streams_call_screencast_buffer_on_next_buffer)
{
    using namespace testing;
    EXPECT_CALL(mock_protobuf_server, screencast_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());

    mcl::ScreencastStream stream(
        nullptr, mock_protobuf_server, 
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)), response);

    auto wh = stream.next_buffer([]{});
    ASSERT_THAT(wh, NotNull());
    EXPECT_FALSE(wh->is_pending());
}
