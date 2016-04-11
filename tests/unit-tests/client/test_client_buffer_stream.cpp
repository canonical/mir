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
#include "src/client/connection_surface_map.h"
#include "src/client/buffer_factory.h"
#include "src/client/protobuf_to_native_buffer.h"

#include "mir/client_platform.h"

#include "mir/test/doubles/null_client_buffer.h"
#include "mir/test/doubles/mock_client_buffer_factory.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/test/doubles/null_logger.h"
#include "mir/test/doubles/mock_protobuf_server.h"
#include "mir/test/doubles/mock_client_buffer.h"
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

ACTION_P(SetResponseError, message)
{
    arg1->set_error(message);
}

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
    std::shared_ptr<void> create_egl_native_window(mcl::EGLNativeSurface * /* surface */) override
    {
        return mt::fake_shared(egl_native_window);
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
    
struct ClientBufferStream : TestWithParam<bool>
{
    ClientBufferStream()
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
        if (legacy_exchange_buffer)
            fill_protobuf_buffer_from_package(protobuf_bs.mutable_buffer(), package);
        return protobuf_bs;
    }
    bool const legacy_exchange_buffer = GetParam();

    void service_requests_for(mcl::ClientBufferStream& bs, unsigned int count)
    {
        for(auto i = 0u; i < count; i++)
        {
            mp::Buffer buffer;
            fill_protobuf_buffer_from_package(&buffer, a_buffer_package());
            buffer.set_width(size.width.as_int());
            buffer.set_height(size.height.as_int());

            async_buffer_arrives(bs, buffer);
        }
    }

    void async_buffer_arrives(mcl::ClientBufferStream& bs, mp::Buffer& buffer)
    {
        if (legacy_exchange_buffer)
        {
            bs.buffer_available(buffer);
        }
        else
        {
            try
            {
                map->buffer(buffer.buffer_id())->received(*mcl::protobuf_to_native_buffer(buffer));
            }
            catch (std::runtime_error& e)
            {
                auto bb = factory->generate_buffer(buffer);
                auto braw = bb.get();
                map->insert(buffer.buffer_id(), std::move(bb)); 
                braw->received();
            }
        }
    }

    std::shared_ptr<mcl::ConnectionSurfaceMap> map{std::make_shared<mcl::ConnectionSurfaceMap>()};
    std::shared_ptr<mcl::BufferFactory> factory{std::make_shared<mcl::BufferFactory>()};
    std::shared_ptr<MirWaitHandle> wait_handle;
    testing::NiceMock<mtd::MockClientBufferFactory> mock_factory;
    mtd::StubClientBufferFactory stub_factory;

    testing::NiceMock<mtd::MockProtobufServer> mock_protobuf_server;

    MirPixelFormat const default_pixel_format = mir_pixel_format_argb_8888;
    MirBufferUsage const default_buffer_usage = mir_buffer_usage_hardware;

    std::shared_ptr<mcl::PerfReport> const perf_report = std::make_shared<mcl::NullPerfReport>();

    MirBufferPackage buffer_package = a_buffer_package();
    geom::Size size{buffer_package.width, buffer_package.height};
    mp::BufferStream response = a_protobuf_buffer_stream(
        default_pixel_format, default_buffer_usage, buffer_package);
    size_t nbuffers{3};
};

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

TEST_P(ClientBufferStream, protobuf_requirements)
{
    auto valid_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, a_buffer_package());
    EXPECT_NO_THROW({
        mcl::BufferStream bs(
            nullptr, wait_handle, mock_protobuf_server,
            std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
            map, factory,
            valid_bs, perf_report, "", size, nbuffers);
    });
    valid_bs.clear_buffer();
    EXPECT_NO_THROW({
        mcl::BufferStream bs(
            nullptr, wait_handle, mock_protobuf_server,
            std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
            map, factory,
            valid_bs, perf_report, "", size, nbuffers);
    });

    auto error_bs = valid_bs;
    error_bs.set_error("An error");
    EXPECT_THROW({
        mcl::BufferStream bs(
            nullptr, wait_handle, mock_protobuf_server,
            std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
            map, factory,
            error_bs, perf_report, "", size, nbuffers);
    }, std::runtime_error);
    
    auto no_id_bs = valid_bs;
    no_id_bs.clear_id();
    EXPECT_THROW({
        mcl::BufferStream bs(
            nullptr, wait_handle, mock_protobuf_server,
            std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
            map, factory,
            no_id_bs, perf_report, "", size, nbuffers);
    }, std::runtime_error);
}

TEST_P(ClientBufferStream, uses_buffer_message_from_server)
{
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(std::make_shared<mtd::NullClientBuffer>()));
    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);
}

TEST_P(ClientBufferStream, producer_streams_call_submit_buffer_on_next_buffer)
{
    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    bs.next_buffer([]{});
}

TEST_P(ClientBufferStream, invokes_callback_on_next_buffer)
{
    mp::Buffer buffer;
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);
    ON_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillByDefault(DoAll(
            mtd::RunProtobufClosure(),
            InvokeWithoutArgs([&bs, &buffer]{ bs.buffer_available(buffer);})));

    bool callback_invoked = false;
    bs.next_buffer([&callback_invoked](){ callback_invoked = true; })->wait_for_all();
    EXPECT_EQ(callback_invoked, true);
}

TEST_P(ClientBufferStream, returns_correct_surface_parameters)
{
    int const width = 73;
    int const height = 32;
    response.mutable_buffer()->set_width(width);
    response.mutable_buffer()->set_height(height);

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    auto params = bs.get_parameters();

    EXPECT_STREQ("", params.name);
    EXPECT_EQ(width, params.width);
    EXPECT_EQ(height, params.height);
    EXPECT_EQ(default_pixel_format, params.pixel_format);
    EXPECT_EQ(default_buffer_usage, params.buffer_usage);
}

TEST_P(ClientBufferStream, returns_current_client_buffer)
{
    auto const client_buffer_1 = std::make_shared<mtd::NullClientBuffer>(size);
    auto const client_buffer_2 = std::make_shared<mtd::NullClientBuffer>(size);
    auto buffer_package_1 = a_buffer_package();
    auto buffer_package_2 = a_buffer_package();

    mp::Buffer protobuf_buffer_1;
    mp::Buffer protobuf_buffer_2;
    fill_protobuf_buffer_from_package(&protobuf_buffer_1, buffer_package_1);
    fill_protobuf_buffer_from_package(&protobuf_buffer_2, buffer_package_2);
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage,
        buffer_package_1);
    
    Sequence seq;
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package_1),_,_))
        .InSequence(seq)
        .WillOnce(Return(client_buffer_1));
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package_2),_,_))
        .InSequence(seq)
        .WillOnce(Return(client_buffer_2));

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        protobuf_bs, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);
    EXPECT_EQ(client_buffer_1, bs.get_current_buffer());

    async_buffer_arrives(bs, protobuf_buffer_2);
    bs.next_buffer([]{});
    EXPECT_EQ(client_buffer_2, bs.get_current_buffer());
}

TEST_P(ClientBufferStream, caches_width_and_height_in_case_of_partial_updates)
{
    auto const client_buffer_1 = std::make_shared<mtd::NullClientBuffer>(size);
    auto const client_buffer_2 = std::make_shared<mtd::NullClientBuffer>(size);
    auto buffer_package_1 = a_buffer_package();
    auto buffer_package_2 = a_buffer_package();
    auto protobuf_bs = a_protobuf_buffer_stream(default_pixel_format, default_buffer_usage, buffer_package_1);
    mp::Buffer protobuf_buffer_2;
    fill_protobuf_buffer_from_package(&protobuf_buffer_2, buffer_package_2);
    auto expected_size = geom::Size{buffer_package_1.width, buffer_package_1.height};

    Sequence seq;
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package_1),expected_size,_))
        .InSequence(seq)
        .WillOnce(Return(client_buffer_1));
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package_2),expected_size,_))
        .InSequence(seq)
        .WillOnce(Return(client_buffer_2));

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        protobuf_bs, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);
    EXPECT_EQ(client_buffer_1, bs.get_current_buffer());
    async_buffer_arrives(bs, protobuf_buffer_2);
    bs.next_buffer([]{});
    EXPECT_EQ(client_buffer_2, bs.get_current_buffer());
}

TEST_P(ClientBufferStream, gets_egl_native_window)
{
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    EXPECT_EQ(StubClientPlatform::egl_native_window, bs.egl_native_window());
}

TEST_P(ClientBufferStream, map_graphics_region)
{
    mtd::MockClientBuffer mock_client_buffer;
    ON_CALL(mock_client_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(mt::fake_shared(mock_client_buffer)));

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);

    mcl::MemoryRegion expected_memory_region;
    EXPECT_CALL(mock_client_buffer, secure_for_cpu_write())
        .WillOnce(Return(mt::fake_shared(expected_memory_region)));
    EXPECT_EQ(&expected_memory_region, bs.secure_for_cpu_write().get());
}

//lp: #1463873
TEST_P(ClientBufferStream, maps_graphics_region_only_once_per_swapbuffers)
{
    mtd::MockClientBuffer mock_client_buffer;
    ON_CALL(mock_client_buffer, size())
        .WillByDefault(Return(size));
    ON_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillByDefault(Return(mt::fake_shared(mock_client_buffer)));
    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, 2);

    mcl::MemoryRegion first_expected_memory_region;
    mcl::MemoryRegion second_expected_memory_region;
    EXPECT_CALL(mock_client_buffer, secure_for_cpu_write())
        .Times(2)
        .WillOnce(Return(mt::fake_shared(first_expected_memory_region)))
        .WillOnce(Return(mt::fake_shared(second_expected_memory_region)));
    EXPECT_EQ(&first_expected_memory_region, bs.secure_for_cpu_write().get());
    bs.secure_for_cpu_write();
    bs.secure_for_cpu_write();

    bs.request_and_wait_for_next_buffer();
    EXPECT_EQ(&second_expected_memory_region, bs.secure_for_cpu_write().get());
    bs.secure_for_cpu_write();
    bs.secure_for_cpu_write();
}

TEST_P(ClientBufferStream, passes_name_to_perf_report)
{
    NiceMock<MockPerfReport> mock_perf_report;
    std::string const name = "a_unique_surface_name";
    EXPECT_CALL(mock_perf_report, name_surface(StrEq(name)));
    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, mt::fake_shared(mock_perf_report), name, size, nbuffers);
}

TEST_P(ClientBufferStream, receives_unsolicited_buffer)
{
    int id = 88;
    mtd::MockClientBuffer mock_client_buffer;
    ON_CALL(mock_client_buffer, size())
        .WillByDefault(Return(size));
    mtd::MockClientBuffer second_mock_client_buffer;
    ON_CALL(second_mock_client_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillOnce(Return(mt::fake_shared(mock_client_buffer)));

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);

    mir::protobuf::Buffer another_buffer_package;
    another_buffer_package.set_buffer_id(id);
    another_buffer_package.set_width(size.width.as_int());
    another_buffer_package.set_height(size.height.as_int());
    EXPECT_CALL(mock_factory, create_buffer(_,_,_))
        .WillOnce(Return(mt::fake_shared(second_mock_client_buffer)));
    EXPECT_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    async_buffer_arrives(bs, another_buffer_package);
    bs.next_buffer([]{});

    EXPECT_THAT(bs.get_current_buffer().get(), Eq(&second_mock_client_buffer));
    EXPECT_THAT(bs.get_current_buffer_id(), Eq(id));
}

TEST_P(ClientBufferStream, waiting_client_can_unblock_on_shutdown)
{
    using namespace std::literals::chrono_literals;
    NiceMock<mtd::MockClientBuffer> mock_client_buffer;
<<<<<<< TREE
    ON_CALL(mock_client_buffer, size())
        .WillByDefault(Return(size));
=======
>>>>>>> MERGE-SOURCE
    ON_CALL(mock_factory, create_buffer(BufferPackageMatches(buffer_package),_,_))
        .WillByDefault(Return(mt::fake_shared(mock_client_buffer)));
    ON_CALL(mock_protobuf_server, submit_buffer(_,_,_))
        .WillByDefault(mtd::RunProtobufClosure());

    std::mutex mutex;
    std::condition_variable cv;
    bool started{false};

    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    auto never_serviced_request = std::async(std::launch::async,[&] {
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            started = true;
            cv.notify_all();
        }
        bs.request_and_wait_for_next_buffer();
    });

    std::unique_lock<decltype(mutex)> lk(mutex);
    EXPECT_TRUE(cv.wait_for(lk, 4s, [&]{ return started; }));

    bs.buffer_unavailable();

    EXPECT_THAT(never_serviced_request.wait_for(4s), Ne(std::future_status::timeout));

    EXPECT_THROW({
        bs.request_and_wait_for_next_buffer();
    }, std::exception);
}

TEST_P(ClientBufferStream, invokes_callback_on_buffer_available_before_wait_handle_has_result)
{
    MirWaitHandle* wh{nullptr};
    bool wait_handle_has_result_in_callback = false;

    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    wh = bs.next_buffer(
        [&]
        {
            if (wh)
                wait_handle_has_result_in_callback = wh->has_result();
        });

    EXPECT_FALSE(wait_handle_has_result_in_callback);
}

TEST_P(ClientBufferStream, invokes_callback_on_buffer_unavailable_before_wait_handle_has_result)
{
    MirWaitHandle* wh{nullptr};
    bool wait_handle_has_result_in_callback = false;

    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    wh = bs.next_buffer(
        [&]
        {
            if (wh)
                wait_handle_has_result_in_callback = wh->has_result();
        });

    bs.buffer_unavailable();
    EXPECT_FALSE(wait_handle_has_result_in_callback);
}

TEST_P(ClientBufferStream, configures_swap_interval)
{
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    EXPECT_CALL(mock_protobuf_server, configure_buffer_stream(_,_,_));
    bs.set_swap_interval(0);
}

TEST_P(ClientBufferStream, sets_swap_interval_requested)
{
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    bs.set_swap_interval(1);
    EXPECT_EQ(1, bs.swap_interval());

    bs.set_swap_interval(0);
    EXPECT_EQ(0, bs.swap_interval());
}

TEST_P(ClientBufferStream, environment_overrides_requested_swap_interval)
{
    setenv("MIR_CLIENT_FORCE_SWAP_INTERVAL", "0", 1);
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    bs.set_swap_interval(1);
    EXPECT_EQ(0, bs.swap_interval());
    unsetenv("MIR_CLIENT_FORCE_SWAP_INTERVAL");
}

MATCHER_P(StreamConfigScaleIs, val, "")
{
    if (!arg->has_scale() || !val.has_scale())
        return false;
    auto const id_matches = arg->id().value() == val.id().value();
    auto const tolerance = 0.01f;
    auto const scale_matches = 
        ((arg->scale() + tolerance >= val.scale()) &&
         (arg->scale() - tolerance <= val.scale()));
    return id_matches && scale_matches; 
}

TEST_P(ClientBufferStream, configures_scale)
{
    if (!legacy_exchange_buffer) return;
    mcl::BufferStream bs{
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(stub_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers};
    service_requests_for(bs, mock_protobuf_server.alloc_count);

    float scale = 2.1;
    mp::StreamConfiguration expected_config;
    expected_config.set_scale(scale);
    expected_config.mutable_id()->set_value(1);
    EXPECT_CALL(mock_protobuf_server, configure_buffer_stream(StreamConfigScaleIs(expected_config),_,_));
    bs.set_scale(scale);
}

TEST_P(ClientBufferStream, returns_correct_surface_parameters_with_nondefault_format)
{
    auto format = mir_pixel_format_bgr_888;
    response.set_pixel_format(format);
    EXPECT_CALL(mock_factory, create_buffer(_,_,format))
        .WillRepeatedly(Return(std::make_shared<mtd::NullClientBuffer>()));
    mcl::BufferStream bs(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    service_requests_for(bs, 1);
    auto params = bs.get_parameters();
    EXPECT_THAT(params.pixel_format, Eq(format));
}

TEST_P(ClientBufferStream, keeps_accurate_buffer_id)
{
    ON_CALL(mock_factory, create_buffer(_,_,_))
        .WillByDefault(Return(std::make_shared<mtd::NullClientBuffer>(size)));
    mcl::BufferStream stream(
        nullptr, wait_handle, mock_protobuf_server,
        std::make_shared<StubClientPlatform>(mt::fake_shared(mock_factory)),
        map, factory,
        response, perf_report, "", size, nbuffers);
    for(auto i = 0u; i < 2; i++)
    {
        mp::Buffer buffer;
        fill_protobuf_buffer_from_package(&buffer, a_buffer_package());
        buffer.set_buffer_id(i+10);
        buffer.set_width(size.width.as_int());
        buffer.set_height(size.height.as_int());
        async_buffer_arrives(stream, buffer);
    }

    EXPECT_THAT(stream.get_current_buffer_id(), Eq(10));
}

INSTANTIATE_TEST_CASE_P(BufferSemanticsMode, ClientBufferStream, Bool());
