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

namespace
{

struct MockProtobufServer : public mclr::DisplayServer
{
    MockProtobufServer() : mclr::DisplayServer(nullptr) {}

    MOCK_METHOD3(screencast_buffer,
                 void(
                      mp::ScreencastId const* /*request*/,
                      mp::Buffer* /*response*/,
                      google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(submit_buffer,
                 void(
                      mp::BufferRequest const* /*request*/,
                      mp::Void* /*response*/,
                      google::protobuf::Closure* /*done*/));
    MOCK_METHOD3(exchange_buffer,
                 void(
                      mp::BufferRequest const* /*request*/,
                      mp::Buffer* /*response*/,
                      google::protobuf::Closure* /*done*/));
};

// TODO: Deduplicate this class
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
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
};

EGLNativeWindowType StubClientPlatform::egl_native_window{
    reinterpret_cast<EGLNativeWindowType>(&StubClientPlatform::egl_native_window)};

struct ClientBufferStreamTest : public testing::Test
{
    testing::NiceMock<mtd::MockClientBufferFactory> mock_client_buffer_factory;
    mtd::StubClientBufferFactory stub_client_buffer_factory;

    testing::NiceMock<MockProtobufServer> mock_protobuf_server;

    MirPixelFormat const default_pixel_format = mir_pixel_format_argb_8888;
    MirBufferUsage const default_buffer_usage = mir_buffer_usage_hardware;

    std::shared_ptr<mcl::PerfReport> const perf_report = std::make_shared<mcl::NullPerfReport>();

    std::shared_ptr<mcl::BufferStream> make_buffer_stream(mp::BufferStream const& protobuf_bs,
        mcl::BufferStreamMode mode=mcl::BufferStreamMode::Producer)
    {
        return make_buffer_stream(protobuf_bs, stub_client_buffer_factory, mode);
    }
    std::shared_ptr<mcl::BufferStream> make_buffer_stream(mp::BufferStream const& protobuf_bs,
        mcl::ClientBufferFactory& buffer_factory,
        mcl::BufferStreamMode mode=mcl::BufferStreamMode::Producer)
    {
        return std::make_shared<mcl::BufferStream>(nullptr, mock_protobuf_server, mode,
            std::make_shared<StubClientPlatform>(mt::fake_shared(buffer_factory)), protobuf_bs, perf_report, "");
    }
};

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
    
MirBufferPackage a_buffer_package()
{
    MirBufferPackage bp;
    
    // Could be randomized
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
    arg2->Run();
}

ACTION_P(SetBufferInfoFromPackage, buffer_package)
{
    arg2->set_buffer_id(unique_buffer_id++);
    arg2->set_width(buffer_package.width);
    arg2->set_height(buffer_package.height);
}

ACTION_P(SetPartialBufferInfoFromPackage, buffer_package)
{
    arg2->set_buffer_id(unique_buffer_id++);
}

}

TEST_F(ClientBufferStreamTest, protobuf_requirements)
{
    auto valid_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, a_buffer_package());
    
    EXPECT_NO_THROW({
        make_buffer_stream(valid_bs);
    });
    
    auto error_bs = valid_bs;
    error_bs.set_error("An error");
    EXPECT_THROW({
        make_buffer_stream(error_bs);
    }, std::runtime_error);
    
    auto no_id_bs = valid_bs;
    no_id_bs.clear_id();
    EXPECT_THROW({
        make_buffer_stream(no_id_bs);
    }, std::runtime_error);


    auto no_buffer_bs = valid_bs;
    no_buffer_bs.clear_buffer();
    EXPECT_THROW({
        make_buffer_stream(no_buffer_bs);
    }, std::runtime_error);
}

TEST_F(ClientBufferStreamTest, uses_buffer_message_from_server)
{
    using namespace ::testing;

    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package);

    EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(std::make_shared<mtd::NullClientBuffer>()));

    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);
}

TEST_F(ClientBufferStreamTest, producer_streams_call_submit_buffer_on_next_buffer)
{
    using namespace ::testing;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());

    auto bs = make_buffer_stream(protobuf_bs, mcl::BufferStreamMode::Producer);
    
    bs->next_buffer([]{});
}

TEST_F(ClientBufferStreamTest, consumer_streams_call_screencast_buffer_on_next_buffer)
{
    using namespace ::testing;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    EXPECT_CALL(mock_protobuf_server, screencast_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());

    auto bs = make_buffer_stream(protobuf_bs, mcl::BufferStreamMode::Consumer);
    
    bs->next_buffer([]{});
}

TEST_F(ClientBufferStreamTest, invokes_callback_on_next_buffer)
{
    using namespace ::testing;
    
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    mp::Buffer buffer;
    auto bs = make_buffer_stream(protobuf_bs, mcl::BufferStreamMode::Producer);
    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(DoAll(
            RunProtobufClosure(),
            InvokeWithoutArgs([&bs, &buffer]{ bs->buffer_available(buffer);})));

    bool callback_invoked = false;
    bs->next_buffer([&callback_invoked](){ callback_invoked = true; })->wait_for_all();
    EXPECT_EQ(callback_invoked, true);
}

TEST_F(ClientBufferStreamTest, returns_correct_surface_parameters)
{
   int const width = 73, height = 32;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());
    protobuf_bs.mutable_buffer()->set_width(width);
    protobuf_bs.mutable_buffer()->set_height(height);

    auto bs = make_buffer_stream(protobuf_bs);
    auto params = bs->get_parameters();

    EXPECT_STREQ("", params.name);
    EXPECT_EQ(width, params.width);
    EXPECT_EQ(height, params.height);
    EXPECT_EQ(default_pixel_format, params.pixel_format);
    EXPECT_EQ(default_buffer_usage, params.buffer_usage);
}

TEST_F(ClientBufferStreamTest, returns_current_client_buffer)
{
    using namespace ::testing;
    ON_CALL(mock_protobuf_server, submit_buffer( _, _, _))
        .WillByDefault(RunProtobufClosure());

    auto const client_buffer_1 = std::make_shared<mtd::NullClientBuffer>();
    auto const client_buffer_2 = std::make_shared<mtd::NullClientBuffer>();
    auto buffer_package_1 = a_buffer_package();
    auto buffer_package_2 = a_buffer_package();
    mp::Buffer protobuf_buffer_1;
    mp::Buffer protobuf_buffer_2;
    fill_protobuf_buffer_from_package(&protobuf_buffer_1, buffer_package_1);
    fill_protobuf_buffer_from_package(&protobuf_buffer_2, buffer_package_2);
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package_1);
    
    {
        InSequence seq;
        EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package_1),_,_))
            .Times(1).WillOnce(Return(client_buffer_1));
        EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package_2),_,_))
            .Times(1).WillOnce(Return(client_buffer_2));
    }

    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);
    EXPECT_EQ(client_buffer_1, bs->get_current_buffer());
    bs->buffer_available(protobuf_buffer_2);
    bs->next_buffer([]{});
    EXPECT_EQ(client_buffer_2, bs->get_current_buffer());
}

TEST_F(ClientBufferStreamTest, caches_width_and_height_in_case_of_partial_updates)
{
    using namespace ::testing;

    auto const client_buffer_1 = std::make_shared<mtd::NullClientBuffer>(),
        client_buffer_2 = std::make_shared<mtd::NullClientBuffer>();

    auto buffer_package_1 = a_buffer_package();
    auto buffer_package_2 = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package_1);
    mp::Buffer protobuf_buffer_2;
    fill_protobuf_buffer_from_package(&protobuf_buffer_2, buffer_package_2);
        
    // Here we us SetPartialBufferInfoFromPackage and do not fill in width and height
    EXPECT_CALL(mock_protobuf_server, submit_buffer( _, _, _))
        .WillOnce(RunProtobufClosure());

    auto expected_size = geom::Size{buffer_package_1.width, buffer_package_1.height};

    {
        InSequence seq;
        EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package_1), expected_size, _))
            .Times(1).WillOnce(Return(client_buffer_1));
        EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package_2), expected_size, _))
            .Times(1).WillOnce(Return(client_buffer_2));
   }

    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);

    EXPECT_EQ(client_buffer_1, bs->get_current_buffer());
    bs->buffer_available(protobuf_buffer_2);
    bs->next_buffer([]{});
    EXPECT_EQ(client_buffer_2, bs->get_current_buffer());
}

TEST_F(ClientBufferStreamTest, gets_egl_native_window)
{
    using namespace ::testing;

    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        a_buffer_package());

    auto bs = make_buffer_stream(protobuf_bs);
    auto egl_native_window = bs->egl_native_window();

    EXPECT_EQ(StubClientPlatform::egl_native_window, egl_native_window);
}

TEST_F(ClientBufferStreamTest, map_graphics_region)
{
    using namespace ::testing;

    MockClientBuffer mock_client_buffer;

    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package);

    EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(mt::fake_shared(mock_client_buffer)));

    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);

    mcl::MemoryRegion expected_memory_region;
    EXPECT_CALL(mock_client_buffer, secure_for_cpu_write()).Times(1)
        .WillOnce(Return(mt::fake_shared(expected_memory_region)));
     
    EXPECT_EQ(&expected_memory_region, bs->secure_for_cpu_write().get());
}

TEST_F(ClientBufferStreamTest, passes_name_to_perf_report)
{
    using namespace ::testing;

    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, buffer_package);

    NiceMock<MockPerfReport> mock_perf_report;
    std::string const name = "a_unique_surface_name";

    EXPECT_CALL(mock_perf_report, name_surface(StrEq(name))).Times(1);

    auto bs = std::make_shared<mcl::BufferStream>(
        nullptr, mock_protobuf_server, mcl::BufferStreamMode::Producer,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_client_buffer_factory)),
        protobuf_bs, mt::fake_shared(mock_perf_report), name);
}

TEST_F(ClientBufferStreamTest, receives_unsolicited_buffer)
{
    using namespace ::testing;
    int id = 88;
    MockClientBuffer mock_client_buffer;
    MockClientBuffer second_mock_client_buffer;
    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, buffer_package);
    EXPECT_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(mt::fake_shared(mock_client_buffer)));
    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);

    mir::protobuf::Buffer another_buffer_package;
    another_buffer_package.set_buffer_id(id);
    EXPECT_CALL(mock_client_buffer_factory, create_buffer(_,_,_))
        .WillOnce(Return(mt::fake_shared(second_mock_client_buffer)));
    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());
    bs->buffer_available(another_buffer_package);
    bs->next_buffer([]{});

    EXPECT_THAT(bs->get_current_buffer().get(), Eq(&second_mock_client_buffer));
    EXPECT_THAT(bs->get_current_buffer_id(), Eq(id));
}

TEST_F(ClientBufferStreamTest, waiting_client_can_unblock_on_shutdown)
{
    using namespace ::testing;
    using namespace std::literals::chrono_literals;
    MockClientBuffer mock_client_buffer;
    MirBufferPackage buffer_package = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, buffer_package);
    ON_CALL(mock_client_buffer_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillByDefault(Return(mt::fake_shared(mock_client_buffer)));
    ON_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillByDefault(RunProtobufClosure());

    std::mutex mutex;
    std::condition_variable cv;
    bool started{false};

    auto bs = make_buffer_stream(protobuf_bs, mock_client_buffer_factory);
    auto never_serviced_request = std::async(std::launch::async,[&] {
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            started = true;
            cv.notify_all();
        }
        bs->request_and_wait_for_next_buffer();
    });

    std::unique_lock<decltype(mutex)> lk(mutex);
    EXPECT_TRUE(cv.wait_for(lk, 4s, [&]{ return started; }));

    bs->buffer_unavailable();

    EXPECT_THAT(never_serviced_request.wait_for(4s), Ne(std::future_status::timeout));

    EXPECT_THROW({
        bs->request_and_wait_for_next_buffer();
    }, std::runtime_error);
}

TEST_F(ClientBufferStreamTest, invokes_callback_on_buffer_available_before_wait_handle_has_result)
{
    using namespace ::testing;

    MirWaitHandle* wh{nullptr};
    bool wait_handle_has_result_in_callback = false;

    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());

    auto const protobuf_bs = a_protobuf_buffer_stream(
        default_pixel_format, default_buffer_usage, a_buffer_package());
    auto const bs = make_buffer_stream(protobuf_bs);

    wh = bs->next_buffer(
        [&]
        {
            wait_handle_has_result_in_callback = wh->has_result();
        });

    bs->buffer_available(mp::Buffer{});

    EXPECT_FALSE(wait_handle_has_result_in_callback);
}

TEST_F(ClientBufferStreamTest, invokes_callback_on_buffer_unavailable_before_wait_handle_has_result)
{
    using namespace ::testing;

    MirWaitHandle* wh{nullptr};
    bool wait_handle_has_result_in_callback = false;

    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(RunProtobufClosure());

    auto const protobuf_bs = a_protobuf_buffer_stream(
        default_pixel_format, default_buffer_usage, a_buffer_package());
    auto const bs = make_buffer_stream(protobuf_bs);

    wh = bs->next_buffer(
        [&]
        {
            wait_handle_has_result_in_callback = wh->has_result();
        });

    bs->buffer_unavailable();

    EXPECT_FALSE(wait_handle_has_result_in_callback);
}
