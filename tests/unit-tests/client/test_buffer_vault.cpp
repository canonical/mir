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
#include "src/client/buffer_factory.h"
#include "src/client/connection_surface_map.h"
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
struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
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
    MOCK_METHOD1(submit_buffer, void(mcl::Buffer&));
    MOCK_METHOD0(disconnected, void());
};

void ignore(MirPresentationChain*, MirBuffer*, void*)
{
}

struct BufferVault : public testing::Test
{
    BufferVault() :
        surface_map(std::make_shared<mcl::ConnectionSurfaceMap>())
    {
        package.set_width(size.width.as_int());
        package.set_height(size.height.as_int());
        package2.set_width(size.width.as_int());
        package2.set_height(size.height.as_int());
        package3.set_width(size.width.as_int());
        package3.set_height(size.height.as_int());
        package4.set_width(new_size.width.as_int());
        package4.set_height(new_size.height.as_int());

        package.set_buffer_id(1);
        package2.set_buffer_id(2);
        package3.set_buffer_id(3);
        package4.set_buffer_id(4);

        auto buffer1 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*buffer1, size())
            .WillByDefault(Return(size));
        auto buffer2 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*buffer2, size())
            .WillByDefault(Return(size));
        auto buffer3 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*buffer3, size())
            .WillByDefault(Return(size));
        auto buffer4 = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*buffer4, size())
            .WillByDefault(Return(new_size));

        auto b1 = std::make_shared<mcl::Buffer>(
            ignore, nullptr, package.buffer_id(), buffer1, nullptr, mir_buffer_usage_software);
        auto b2 = std::make_shared<mcl::Buffer>(
            ignore, nullptr, package2.buffer_id(), buffer2, nullptr, mir_buffer_usage_software);
        auto b3 = std::make_shared<mcl::Buffer>(
            ignore, nullptr, package3.buffer_id(), buffer3, nullptr, mir_buffer_usage_software);
        auto b4 = std::make_shared<mcl::Buffer>(
            ignore, nullptr, package4.buffer_id(), buffer4, nullptr, mir_buffer_usage_software);
        surface_map->insert(package.buffer_id(), b1);
        surface_map->insert(package2.buffer_id(), b2);
        surface_map->insert(package3.buffer_id(), b3);
        surface_map->insert(package4.buffer_id(), b4);
        b1->received();
        b2->received();
        b3->received();
        b4->received();
    }

    std::unique_ptr<mcl::BufferVault> make_vault()
    {
        return std::make_unique<mcl::BufferVault>(
            mt::fake_shared(mock_platform_factory),
            mt::fake_shared(buffer_factory),
            mt::fake_shared(mock_requests),
            surface_map,
            size, format, usage, initial_nbuffers);
    }

    unsigned int initial_nbuffers {3};
    geom::Size size{271, 314};
    float scale = 2.0f;
    geom::Size new_size{ size * scale };
    MirPixelFormat format{mir_pixel_format_abgr_8888};
    int usage{0};
    mg::BufferProperties initial_properties{
        geom::Size{271,314}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    NiceMock<MockClientBufferFactory> mock_platform_factory;
    NiceMock<MockServerRequests> mock_requests;
    mcl::BufferFactory buffer_factory;
    std::shared_ptr<mcl::ConnectionSurfaceMap> surface_map;
    mp::Buffer package;
    mp::Buffer package2;
    mp::Buffer package3;
    mp::Buffer package4;
};

struct StartedBufferVault : BufferVault
{
    StartedBufferVault()
    {
        vault.wire_transfer_inbound(package.buffer_id());
        vault.wire_transfer_inbound(package2.buffer_id());
        vault.wire_transfer_inbound(package3.buffer_id());
    }
    mcl::BufferVault vault{
        mt::fake_shared(mock_platform_factory), mt::fake_shared(buffer_factory),
        mt::fake_shared(mock_requests), surface_map,
        size, format, usage, initial_nbuffers};
};

}

TEST_F(BufferVault, creates_all_buffers_on_start)
{
    EXPECT_CALL(mock_requests, allocate_buffer(size, format, usage))
        .Times(initial_nbuffers);
    make_vault();
}

TEST_F(BufferVault, frees_the_buffers_we_actually_got)
{
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, free_buffer(package2.buffer_id()));
    auto vault = make_vault();

    vault->wire_transfer_inbound(package.buffer_id());
    vault->wire_transfer_inbound(package2.buffer_id());
}

TEST_F(BufferVault, withdrawing_and_never_filling_up_will_timeout)
{
    using namespace std::literals::chrono_literals;

    auto vault = make_vault();
    auto buffer_future = vault->withdraw();
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
    EXPECT_CALL(mock_requests, submit_buffer(Ref(*buffer)));
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer, []{});
}

TEST_F(StartedBufferVault, cant_transfer_if_not_in_acct)
{
    auto buffer = vault.withdraw().get();
    EXPECT_THROW({ 
        vault.wire_transfer_outbound(buffer, []{});
    }, std::logic_error);
}

TEST_F(StartedBufferVault, depositing_external_buffer_throws)
{
    auto platform_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*platform_buffer, size())
        .WillByDefault(Return(size));
    auto buffer = std::make_shared<mcl::Buffer>(
        ignore, nullptr, 0, platform_buffer, nullptr, mir_buffer_usage_software);
    EXPECT_THROW({ 
        vault.deposit(buffer);
    }, std::logic_error);
}

TEST_F(StartedBufferVault, attempt_to_redeposit_throws)
{
    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer, []{});
    EXPECT_THROW({
        vault.deposit(buffer);
    }, std::logic_error);
}

TEST_F(BufferVault, can_transfer_again_when_we_get_the_buffer)
{
    auto vault = make_vault();
    vault->wire_transfer_inbound(package.buffer_id());
    auto buffer = vault->withdraw().get();
    vault->deposit(buffer);
    vault->wire_transfer_outbound(buffer, []{});

    vault->wire_transfer_inbound(package.buffer_id());
    auto buffer2 = vault->withdraw().get();
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
    auto vault = make_vault();

    auto f_buffer1 = vault->withdraw();
    auto f_buffer2 = vault->withdraw();
    vault->wire_transfer_inbound(package.buffer_id());
    vault->wire_transfer_inbound(package2.buffer_id());

    auto buffer1 = f_buffer1.get();
    auto buffer2 = f_buffer2.get();
    EXPECT_THAT(buffer1, Ne(buffer2));
}

TEST_F(BufferVault, destruction_signals_futures)
{
    using namespace std::literals::chrono_literals;
    mcl::NoTLSFuture<std::shared_ptr<mcl::Buffer>> fbuffer;
    {
        auto vault = make_vault();
        fbuffer = vault->withdraw();
    }
    EXPECT_THAT(fbuffer.wait_for(5s), Ne(std::future_status::timeout));
}

TEST_F(BufferVault, ages_buffer_on_deposit)
{
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*mock_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(*mock_buffer, increment_age());
    ON_CALL(mock_platform_factory, create_buffer(_,_,_))
        .WillByDefault(Return(mock_buffer));
    auto b1 = std::make_shared<mcl::Buffer>(
        ignore, nullptr, package.buffer_id(), mock_buffer, nullptr, mir_buffer_usage_software);
    surface_map->insert(package.buffer_id(), b1);

    auto vault = make_vault();
    vault->wire_transfer_inbound(package.buffer_id());
    vault->deposit(vault->withdraw().get());
}

TEST_F(BufferVault, marks_as_submitted_on_transfer)
{
    auto mock_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*mock_buffer, size())
        .WillByDefault(Return(size));
    EXPECT_CALL(*mock_buffer, mark_as_submitted());
    ON_CALL(mock_platform_factory, create_buffer(_,_,_))
        .WillByDefault(Return(mock_buffer));
    auto b1 = std::make_shared<mcl::Buffer>(
        ignore, nullptr, package.buffer_id(), mock_buffer, nullptr, mir_buffer_usage_software);
    surface_map->insert(package.buffer_id(), b1);
    b1->received();

    auto vault = make_vault();
    vault->wire_transfer_inbound(package.buffer_id());

    auto buffer = vault->withdraw().get();
    vault->deposit(buffer);
    vault->wire_transfer_outbound(buffer, []{});
}

TEST_F(StartedBufferVault, reallocates_incoming_buffers_of_incorrect_size_with_immediate_response)
{
    vault.set_size(new_size);

    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_))
        .WillOnce(Invoke(
        [&, this](geom::Size, MirPixelFormat, int)
        {
            vault.wire_transfer_inbound(package4.buffer_id());
        }));

    vault.wire_transfer_inbound(package.buffer_id());
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, reallocates_incoming_buffers_of_incorrect_size_with_delayed_response)
{
    vault.set_size(new_size);

    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_));

    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package4.buffer_id());
    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, withdraw_gives_only_newly_sized_buffers_after_resize)
{
    vault.set_size(new_size);
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package4.buffer_id());

    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, setting_size_frees_unneeded_buffers_right_away)
{
    EXPECT_CALL(mock_requests, free_buffer(_)).Times(3);
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_)).Times(0);
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
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .WillOnce(InvokeWithoutArgs(
            [&]{ vault.wire_transfer_inbound(package4.buffer_id());}));
    auto b1 = vault.withdraw().get();
    auto b2 = vault.withdraw().get();
    vault.set_scale(scale);

    auto b3 = vault.withdraw().get();
    EXPECT_THAT(b3->size(), Eq(new_size));
}

TEST_F(BufferVault, waiting_threads_give_error_if_disconnected)
{
    auto vault = make_vault();

    auto future = vault->withdraw();
    vault->disconnected();
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
        auto vault = make_vault();
        vault->wire_transfer_inbound(package.buffer_id());
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

    int i = initial_nbuffers;
    for (auto& buffer : buffers)
    {
        i++;
        buffer.set_width(new_size.width.as_int());
        buffer.set_height(new_size.height.as_int());
        buffer.set_buffer_id(i);

        auto mcb = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
        ON_CALL(*mcb, size())
            .WillByDefault(Return(new_size));
        auto b = std::make_shared<mcl::Buffer>(
            ignore, nullptr, i, mcb, nullptr, mir_buffer_usage_software);
        surface_map->insert(i, b);
        b->received();
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
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(initial_nbuffers);

    vault.set_scale(scale);

    for(auto i = 0u; i < initial_nbuffers; i++)
    {
        auto b = vault.withdraw().get();
        vault.deposit(b);
    }

    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(BufferVault, rescale_before_initial_buffers_are_serviced_frees_initial_buffers)
{
    auto vault = make_vault();
    vault->set_scale(2.0);

    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(initial_nbuffers);
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(initial_nbuffers);
    vault->wire_transfer_inbound(package.buffer_id());
    vault->wire_transfer_inbound(package2.buffer_id());
    vault->wire_transfer_inbound(package3.buffer_id());
    
}

TEST_F(StartedBufferVault, can_increase_allocation_count)
{
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(1);
    vault.increase_buffer_count();

    Mock::VerifyAndClearExpectations(&mock_requests);
    //no buffers
}

TEST_F(StartedBufferVault, cannot_decrease_allocation_count_below_initial)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(0);
    vault.decrease_buffer_count();
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, can_decrease_allocation_count)
{
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(1);
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(1);
    vault.increase_buffer_count();
    vault.decrease_buffer_count();
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, delayed_decrease_allocation_count)
{
    mp::Buffer requested_buffer;
    requested_buffer.set_width(size.width.as_int());
    requested_buffer.set_height(size.height.as_int());
    requested_buffer.set_buffer_id(4);
    auto extra_native_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    ON_CALL(*extra_native_buffer, size())
        .WillByDefault(Return(size));
    auto extra_buffer = std::make_shared<mcl::Buffer>(
        ignore, nullptr, package4.buffer_id(), extra_native_buffer, nullptr, mir_buffer_usage_software);
    surface_map->insert(package4.buffer_id(), extra_buffer);
    extra_buffer->received();

    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(1);
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()))
        .Times(1);

    vault.increase_buffer_count();
    vault.wire_transfer_inbound(requested_buffer.buffer_id());
    auto b = vault.withdraw().get();
    vault.deposit(b);
    vault.wire_transfer_outbound(b, []{});

    b = vault.withdraw().get();
    vault.deposit(b);
    vault.wire_transfer_outbound(b, []{});

    b = vault.withdraw().get();
    vault.deposit(b);
    vault.wire_transfer_outbound(b, []{});

    b = vault.withdraw().get();
    vault.deposit(b);
    vault.wire_transfer_outbound(b, []{});

    vault.decrease_buffer_count();
    vault.wire_transfer_inbound(package.buffer_id());
    vault.wire_transfer_inbound(package2.buffer_id());
    Mock::VerifyAndClearExpectations(&mock_requests);
}

//LP: #1578159
TEST_F(StartedBufferVault, prefers_buffers_returned_further_in_the_past)
{
    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer, []{});
    auto first_id = buffer->rpc_id();

    buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer, []{});
    auto second_id = buffer->rpc_id();

    vault.wire_transfer_inbound(second_id);
    vault.wire_transfer_inbound(first_id);

    EXPECT_THAT(vault.withdraw().get()->rpc_id(), Ne(first_id));
}

//LP: #1579076
TEST_F(StartedBufferVault, doesnt_let_buffers_stagnate_after_resize)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(3);
    vault.set_size(new_size);

    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_))
        .Times(1);
    auto buffer_future = vault.withdraw();
    vault.wire_transfer_inbound(package4.buffer_id());
    EXPECT_THAT(buffer_future.get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, doesnt_free_buffers_in_the_driver_after_resize)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(2);
    auto buffer = vault.withdraw().get();
    vault.set_size(new_size);
    Mock::VerifyAndClearExpectations(&mock_requests);

    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer, []{});

    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(1);
    vault.wire_transfer_inbound(buffer->rpc_id());
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, doesnt_free_buffers_with_content_after_resize)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(2);
    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.set_size(new_size);
    Mock::VerifyAndClearExpectations(&mock_requests);

    vault.wire_transfer_outbound(buffer, []{});
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(1);
    vault.wire_transfer_inbound(buffer->rpc_id());
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, doesnt_free_buffers_if_size_is_the_same)
{
    EXPECT_CALL(mock_requests, free_buffer(_))
        .Times(0);
    vault.set_size(size);
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, delays_allocation_if_not_needed)
{
    vault.set_size(new_size);

    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_))
        .Times(1)
        .WillOnce(Invoke(
        [&, this](geom::Size, MirPixelFormat, int)
        {
            vault.wire_transfer_inbound(package4.buffer_id());
        }));

    for(auto i = 0u; i < 10u; i++)
    {
        auto buffer = vault.withdraw().get();
        vault.deposit(buffer);
        buffer->received();
        vault.wire_transfer_outbound(buffer, []{});
        vault.wire_transfer_inbound(buffer->rpc_id());
    }
}

TEST_F(StartedBufferVault, can_increase_count_after_resize)
{
    auto num_allocations = 4u;
    EXPECT_CALL(mock_requests, allocate_buffer(_,_,_))
        .Times(num_allocations);

    vault.increase_buffer_count();
    vault.set_size(new_size);

    for(auto i = 0u; i < num_allocations + 1; i++)
        vault.withdraw();
}
