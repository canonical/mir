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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "src/client/client_buffer_depository.h"
#include "src/client/buffer_vault.h"
#include "src/client/surface_map.h"
#include "src/client/connection_surface_map.h"
#include "src/client/buffer_factory.h"
#include "mir/client_buffer_factory.h"
#include "mir/aging_buffer.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/fake_shared.h"
#include "mir_protobuf.pb.h"
#include "mir/test/doubles/mock_client_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mir/test/gmock_fixes.h"
#include <stdexcept>
#include <array>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace mp = mir::protobuf;
namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{
struct MockSurfaceMap : mcl::SurfaceMap
{
    std::shared_ptr<mtd::MockClientBuffer> make_client_buffer()
    {
        auto b = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*b, size())
            .WillByDefault(Return(size));
        return b;
    }

    geom::Size size;
    MockSurfaceMap(geom::Size size) : size(size)
    {
        ON_CALL(*this, buffer(_))
            .WillByDefault(Invoke(
            [this](int id)
            {
                return std::make_shared<mcl::Buffer>(
                    nullptr,nullptr, id , make_client_buffer(), nullptr, mir_buffer_usage_software);
            })); 
    }
    MOCK_CONST_METHOD2(with_surface_do,
        void(mir::frontend::SurfaceId, std::function<void(MirSurface*)> const&));
    MOCK_CONST_METHOD2(with_stream_do,
        void(mir::frontend::BufferStreamId, std::function<void(mcl::ClientBufferStream*)> const&));
    MOCK_CONST_METHOD1(with_all_streams_do,
        void(std::function<void(mcl::ClientBufferStream*)> const&));
    MOCK_CONST_METHOD2(with_buffer_do, bool(int, std::function<void(mcl::Buffer&)> const&));
    MOCK_METHOD2(insert, void(int, std::shared_ptr<mcl::Buffer> const&));
    MOCK_CONST_METHOD1(buffer, std::shared_ptr<mcl::Buffer>(int));
    MOCK_METHOD1(erase, void(int));
}; 

struct MockBufferFactory : mcl::AsyncBufferFactory
{
    MOCK_METHOD1(generate_buffer, std::unique_ptr<mcl::Buffer>(mp::Buffer const&));
    MOCK_METHOD7(expect_buffer, void(
        std::shared_ptr<mcl::ClientBufferFactory> const&,
        MirConnection*,
        geom::Size, MirPixelFormat, MirBufferUsage,
        mir_buffer_callback, void*));
};

struct NativeBufferFactory : public mcl::ClientBufferFactory
{
    NativeBufferFactory()
    {
        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(Invoke([](
                    std::shared_ptr<MirBufferPackage> const&, geom::Size size, MirPixelFormat)
                {
                    auto buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
                    ON_CALL(*buffer, size())
                        .WillByDefault(Return(size));
                    return buffer;
                }));
    }
    MOCK_METHOD3(create_buffer, std::shared_ptr<mcl::ClientBuffer>(
        std::shared_ptr<MirBufferPackage> const&, geom::Size, MirPixelFormat));
};

struct MockServerRequests : mcl::ServerBufferRequests
{
    MOCK_METHOD3(allocate_buffer, void(geom::Size size, MirPixelFormat format, int usage));
    MOCK_METHOD1(free_buffer, void(int));
    MOCK_METHOD2(submit_buffer, void(int, mcl::ClientBuffer&));
    MOCK_METHOD0(disconnected, void());
};

struct BufferVault : public testing::Test
{
    BufferVault()
    {
        package.set_width(size.width.as_int());
        package.set_height(size.height.as_int());
        package2.set_width(size.width.as_int());
        package2.set_height(size.height.as_int());
        package3.set_width(size.width.as_int());
        package3.set_height(size.height.as_int());

        package.set_buffer_id(1);
        package2.set_buffer_id(2);
        package3.set_buffer_id(3);
    }
    unsigned int initial_nbuffers {3};
    geom::Size size{271, 314};
    MirPixelFormat format{mir_pixel_format_abgr_8888};
    int usage{0};
    mg::BufferProperties initial_properties{
        geom::Size{271,314}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    NiceMock<MockBufferFactory> mock_factory;
    NiceMock<MockSurfaceMap> mock_map{size};
    NiceMock<NativeBufferFactory> mock_native_factory;
    NiceMock<MockServerRequests> mock_requests;
    mp::Buffer package;
    mp::Buffer package2;
    mp::Buffer package3;
};

void buffer_cb(MirBuffer*, void*)
{
}

struct StartedBufferVault : BufferVault
{
    StartedBufferVault()
    {
        auto mb1 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        auto mb2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        auto mb3 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*mb1, size()).WillByDefault(Return(size));
        ON_CALL(*mb2, size()).WillByDefault(Return(size));
        ON_CALL(*mb3, size()).WillByDefault(Return(size));
        auto b1 = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, package.buffer_id(),
            mb1, nullptr, mir_buffer_usage_software);
        auto b2 = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, package.buffer_id(),
            mb2, nullptr, mir_buffer_usage_software);
        auto b3 = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, package.buffer_id(),
            mb3, nullptr, mir_buffer_usage_software);
        b1->received();
        b2->received();
        b3->received();
        map->insert(package.buffer_id(),  b1);
        map->insert(package2.buffer_id(), b2);
        map->insert(package3.buffer_id(), b3);


        vault.wire_transfer_inbound(package.buffer_id());
        vault.wire_transfer_inbound(package2.buffer_id());
        vault.wire_transfer_inbound(package3.buffer_id());
    }
    std::shared_ptr<mcl::ConnectionSurfaceMap> map{std::make_shared<mcl::ConnectionSurfaceMap>()};
    mcl::BufferVault vault{
        mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        map, mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers};
};
}

TEST_F(BufferVault, creates_all_buffers_on_start)
{
    EXPECT_CALL(mock_requests, allocate_buffer(size, format, usage))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_))
        .Times(initial_nbuffers);
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
}

TEST_F(BufferVault, frees_the_buffers_we_actually_got)
{
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, free_buffer(package2.buffer_id()));
    EXPECT_CALL(mock_map, erase(An<int>())).Times(2);
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package2.buffer_id());
}

TEST_F(BufferVault, withdrawing_and_never_filling_up_will_timeout)
{
    using namespace std::literals::chrono_literals;

    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
    auto buffer_future = vault.withdraw();
    ASSERT_TRUE(buffer_future.valid());
    EXPECT_THAT(buffer_future.wait_for(20ms), Eq(std::future_status::timeout));
}

TEST_F(StartedBufferVault, withdrawing_gives_a_valid_future)
{
    auto buffer_future = vault.withdraw();
    ASSERT_TRUE(buffer_future.valid());
    EXPECT_THAT(buffer_future.get(), Ne(nullptr));;
}

TEST_F(StartedBufferVault, can_deposit_buffer)
{
    auto buffer = vault.withdraw().get();
    EXPECT_CALL(mock_requests, submit_buffer(_,Ref(*buffer->client_buffer())));
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);
}

TEST_F(StartedBufferVault, cant_transfer_if_not_in_acct)
{
    auto buffer = vault.withdraw().get();
    EXPECT_THROW({ 
        vault.wire_transfer_outbound(buffer);
    }, std::logic_error);
}

TEST_F(StartedBufferVault, depositing_external_buffer_throws)
{
    auto client_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*client_buffer, size())
        .WillByDefault(Return(size));
    auto buffer = std::make_shared<mcl::Buffer>(nullptr, nullptr, 0, client_buffer, nullptr, mir_buffer_usage_software);
    EXPECT_THROW({ 
        vault.deposit(buffer);
    }, std::logic_error);
}

TEST_F(StartedBufferVault, attempt_to_redeposit_throws)
{
    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);
    EXPECT_THROW({
        vault.deposit(buffer);
    }, std::logic_error);
}

TEST_F(BufferVault, can_transfer_again_when_we_get_the_buffer)
{
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_))
        .Times(Exactly(initial_nbuffers));

    auto map = std::make_shared<mcl::ConnectionSurfaceMap>();
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        map, mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);

    
    //EXPECT_CALL(mock_native_factory, create_buffer(_,initial_properties.size,initial_properties.format))
    //    .Times(Exactly(1));

    auto mb = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*mb, size()).WillByDefault(Return(size));
    auto b = std::make_shared<mcl::Buffer>(buffer_cb, nullptr, package.buffer_id(),
        mb, nullptr, mir_buffer_usage_software);
    b->received();
    map->insert(package.buffer_id(), b);

    vault.wire_transfer_inbound(package.buffer_id());



    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);

    //should just activate, not create the buffer
    vault.wire_transfer_inbound(package.buffer_id());
    auto buffer2 = vault.withdraw().get();
    EXPECT_THAT(buffer, Eq(buffer2)); 
}

TEST_F(StartedBufferVault, multiple_draws_get_different_buffer)
{
    auto buffer1 = vault.withdraw().get();
    auto buffer2 = vault.withdraw().get();
    EXPECT_THAT(buffer1, Ne(buffer2));
}

TEST_F(BufferVault, multiple_withdrawals_during_wait_period_get_differing_buffers)
{
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);

    auto f_buffer1 = vault.withdraw();
    auto f_buffer2 = vault.withdraw();
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package2.buffer_id());

    auto buffer1 = f_buffer1.get();
    auto buffer2 = f_buffer2.get();
    EXPECT_THAT(buffer1, Ne(buffer2));
}

TEST_F(BufferVault, destruction_signals_futures)
{
    using namespace std::literals::chrono_literals;
    mcl::NoTLSFuture<std::shared_ptr<mcl::Buffer>> fbuffer;
    {
        mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
            mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
            size, format, usage, initial_nbuffers);
        fbuffer = vault.withdraw();
    }
    EXPECT_THAT(fbuffer.wait_for(5s), Ne(std::future_status::timeout));
}

TEST_F(BufferVault, ages_buffer_on_deposit)
{
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*mock_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(*mock_buffer, increment_age());

    ON_CALL(mock_map, buffer(_))
        .WillByDefault(Invoke(
        [this, &mock_buffer](int id)
        {
            return std::make_shared<mcl::Buffer>(
                nullptr,nullptr, id , mock_buffer, nullptr, mir_buffer_usage_software);
        })); 

    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package.buffer_id());
    vault.deposit(vault.withdraw().get());
}

TEST_F(BufferVault, marks_as_submitted_on_transfer)
{
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*mock_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(*mock_buffer, mark_as_submitted());
    ON_CALL(mock_map, buffer(_))
        .WillByDefault(Invoke(
        [this, &mock_buffer](int id)
        {
            auto b = std::make_shared<mcl::Buffer>(
                buffer_cb ,nullptr, id , mock_buffer, nullptr, mir_buffer_usage_software);
            b->received();
            return b;
        })); 

    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package.buffer_id());

    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);
}

//handy for android's cancelbuffer
TEST_F(StartedBufferVault, can_withdraw_and_deposit)
{
    auto a_few_times = 5u;
    std::vector<std::shared_ptr<mcl::Buffer>> buffers(a_few_times);
    for (auto i = 0u; i < a_few_times; i++)
    {
        buffers[i] = vault.withdraw().get();
        vault.deposit(buffers[i]);
    }
    EXPECT_THAT(buffers, Each(buffers[0]));
}

namespace { void bcb(MirBuffer*, void*) {} }
TEST_F(BufferVault, reallocates_incoming_buffers_of_incorrect_size_with_immediate_response)
{
    mp::Buffer package4;
    geom::Size new_size{80, 100}; 
    auto cbuf = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    auto buf = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf, nullptr, mir_buffer_usage_software);
    ON_CALL(*cbuf, size())
        .WillByDefault(Return(size));
    ON_CALL(mock_map, buffer(package.buffer_id()))
        .WillByDefault(Return(buf));

    auto cbuf2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    auto buf2 = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf2, nullptr, mir_buffer_usage_software);
    ON_CALL(*cbuf2, size())
        .WillByDefault(Return(new_size));
    ON_CALL(mock_map, buffer(4))
        .WillByDefault(Return(buf2));

    mcl::BufferVault vault{
        mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers};

    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_map, erase(package.buffer_id()));
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_))
        .WillOnce(Invoke(
        [&, this](geom::Size, MirPixelFormat, int)
        {
            package4.set_buffer_id(4);
            vault.wire_transfer_inbound(package4.buffer_id());
        }));


    vault.set_size(new_size);
    vault.wire_transfer_inbound(package.buffer_id());

    Mock::VerifyAndClearExpectations(&mock_requests);
    Mock::VerifyAndClearExpectations(&mock_map);
}

TEST_F(BufferVault, reallocates_incoming_buffers_of_incorrect_size_with_delayed_response)
{
    geom::Size new_size{80, 100}; 
    mp::Buffer package4;
    package4.set_width(new_size.width.as_int());
    package4.set_height(new_size.height.as_int());
    package4.set_buffer_id(4);
    auto cbuf = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    auto buf = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf, nullptr, mir_buffer_usage_software);
    ON_CALL(*cbuf, size())
        .WillByDefault(Return(size));
    ON_CALL(mock_map, buffer(package.buffer_id()))
        .WillByDefault(Return(buf));

    auto cbuf2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    auto buf2 = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf2, nullptr, mir_buffer_usage_software);
    ON_CALL(*cbuf2, size())
        .WillByDefault(Return(new_size));
    ON_CALL(mock_map, buffer(4))
        .WillByDefault(Return(buf2));

    EXPECT_CALL(mock_map, erase(An<int>()));
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_));

    mcl::BufferVault vault{
        mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, 0};

    vault.set_size(new_size);
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package4.buffer_id());
    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
    Mock::VerifyAndClearExpectations(&mock_map);
}

TEST_F(StartedBufferVault, withdraw_gives_only_newly_sized_buffers_after_resize)
{
    mp::Buffer package4;
    geom::Size new_size{80, 100};
    package4.set_width(new_size.width.as_int());
    package4.set_height(new_size.height.as_int());
    package4.set_buffer_id(4);
 
    mcl::BufferVault vault{
        mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, 0};

    auto cbuf2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    auto buf2 = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf2, nullptr, mir_buffer_usage_software);
    ON_CALL(*cbuf2, size())
        .WillByDefault(Return(new_size));
    ON_CALL(mock_map, buffer(4))
        .WillByDefault(Return(buf2));
    vault.set_size(new_size);
    vault.wire_transfer_inbound(package4.buffer_id());

    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
    Mock::VerifyAndClearExpectations(&mock_map);
}

TEST_F(StartedBufferVault, simply_setting_size_triggers_no_server_interations)
{
    EXPECT_CALL(mock_requests, free_buffer(_)).Times(0);
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_)).Times(0);
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_)).Times(0);
    auto const cycles = 30u;
    geom::Size new_size(80, 100);
    for(auto i = 0u; i < cycles; i++)
    {
        new_size = geom::Size(geom::Width(i), new_size.height);
        vault.set_size(new_size);
    }
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, scaling_resizes_buffers_right_away)
{
    mp::Buffer package4;
    float scale = 2.0f;
    geom::Size new_size = size * scale;
    package4.set_width(new_size.width.as_int());
    package4.set_height(new_size.height.as_int());
    package4.set_buffer_id(4);

    auto cbuf2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*cbuf2, size()).WillByDefault(Return(new_size));
    auto buf2 = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf2, nullptr, mir_buffer_usage_software);

    map->insert(4, buf2);

    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .WillOnce(InvokeWithoutArgs(
            [&]{vault.wire_transfer_inbound(package4.buffer_id());}));
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_));


    auto b1 = vault.withdraw().get();
    auto b2 = vault.withdraw().get();
    vault.set_scale(scale);

    auto b3 = vault.withdraw().get();
    EXPECT_THAT(b3->size(), Eq(new_size));
}

TEST_F(BufferVault, waiting_threads_give_error_if_disconnected)
{
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);

    auto future = vault.withdraw();
    vault.disconnected();
    EXPECT_THROW({
        future.get();
    }, std::exception);
}

TEST_F(BufferVault, makes_sure_rpc_calls_exceptions_are_caught_in_destructor)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    EXPECT_NO_THROW({
        mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
            mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
            size, format, usage, initial_nbuffers);
        vault.wire_transfer_inbound(package.buffer_id());
    });
}

TEST_F(StartedBufferVault, skips_free_buffer_rpc_calls_if_disconnected)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(0);
    vault.disconnected();
}

TEST_F(StartedBufferVault, buffer_count_remains_the_same_after_scaling)
{
    std::array<mp::Buffer, 3> buffers;
    float scale = 2.0f;
    geom::Size new_size = size * scale;

    int i = initial_nbuffers;
    for (auto& buffer : buffers)
    {
        auto cbuf2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*cbuf2, size()).WillByDefault(Return(new_size));
        auto buf2 = std::make_shared<mcl::Buffer>( bcb ,nullptr, 0, cbuf2, nullptr, mir_buffer_usage_software);
        buf2->received();
        buffer.set_width(new_size.width.as_int());
        buffer.set_height(new_size.height.as_int());
        i++;
        buffer.set_buffer_id(i);
        map->insert(i, buf2);
    }



    //make sure we alloc 3 new ones and free 3 old ones
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(initial_nbuffers)
        .WillOnce(InvokeWithoutArgs(
            [&]{vault.wire_transfer_inbound(buffers[0].buffer_id());}))
        .WillOnce(InvokeWithoutArgs(
            [&]{vault.wire_transfer_inbound(buffers[1].buffer_id());}))
        .WillOnce(InvokeWithoutArgs(
            [&]{vault.wire_transfer_inbound(buffers[2].buffer_id());}));
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(initial_nbuffers);

    auto buffer = vault.withdraw().get();
    vault.set_scale(scale);
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);

    map->buffer(package.buffer_id())->received();
    vault.wire_transfer_inbound(package.buffer_id());

    for(auto i = 0; i < 100; i++)
    {
        auto b = vault.withdraw().get();
        EXPECT_THAT(b->size(), Eq(new_size));
        vault.deposit(b);
        vault.wire_transfer_outbound(b);
        map->buffer(buffers[(i+1)%3].buffer_id())->received();
        vault.wire_transfer_inbound(buffers[(i+1)%3].buffer_id());
    }
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(BufferVault, rescale_before_initial_buffers_are_serviced_frees_initial_buffers)
{
    mcl::BufferVault vault(mt::fake_shared(mock_native_factory), mt::fake_shared(mock_requests),
        mt::fake_shared(mock_map), mt::fake_shared(mock_factory),
        size, format, usage, initial_nbuffers);
    vault.set_scale(2.0);

    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_factory, expect_buffer(_,_,_,_,_,_,_))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_map, erase(An<int>()))
        .Times(initial_nbuffers);
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package2.buffer_id());
    vault.wire_transfer_inbound(package3.buffer_id());
}
