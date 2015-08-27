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
#include "mir/client_buffer_factory.h"
#include "mir/aging_buffer.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/fake_shared.h"
#include "mir_protobuf.pb.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace mp = mir::protobuf;
using namespace testing;

namespace
{
struct MockBuffer : public mcl::AgingBuffer
{
    MockBuffer(geom::Size sz)
    {
        ON_CALL(*this, size())
            .WillByDefault(Return(sz));
        ON_CALL(*this, mark_as_submitted())
            .WillByDefault(Invoke([this](){this->AgingBuffer::mark_as_submitted();}));
    }

    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<mir::graphics::NativeBuffer>());
    MOCK_METHOD1(update_from, void(MirBufferPackage const&));
    MOCK_METHOD1(fill_update_msg, void(MirBufferPackage&));
    MOCK_METHOD0(increment_age, void());
    MOCK_CONST_METHOD0(age, uint32_t());
};

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
    {
        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(Invoke([](
                    std::shared_ptr<MirBufferPackage> const&, geom::Size size, MirPixelFormat)
                {
                    return std::make_shared<NiceMock<MockBuffer>>(size);
                }));
    }
    MOCK_METHOD3(create_buffer, std::shared_ptr<mcl::ClientBuffer>(
        std::shared_ptr<MirBufferPackage> const&, geom::Size, MirPixelFormat));
};

struct MockServerRequests : mcl::ServerBufferRequests
{
    MOCK_METHOD3(allocate_buffer, void(geom::Size size, MirPixelFormat format, int usage));
    MOCK_METHOD1(free_buffer, void(int));
    MOCK_METHOD1(submit_buffer, void(mcl::ClientBuffer&));
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
    NiceMock<MockClientBufferFactory> mock_factory;
    NiceMock<MockServerRequests> mock_requests;
    mp::Buffer package;
    mp::Buffer package2;
    mp::Buffer package3;
};

struct StartedBufferVault : BufferVault
{
    StartedBufferVault()
    {
        vault.wire_transfer_inbound(package);
        vault.wire_transfer_inbound(package2);
        vault.wire_transfer_inbound(package3);
    }
    mcl::BufferVault vault{
        mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers};
};
}

TEST_F(BufferVault, creates_all_buffers_on_start)
{
    EXPECT_CALL(mock_requests, allocate_buffer(size, format, usage))
        .Times(initial_nbuffers);
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
}

TEST_F(BufferVault, frees_the_buffers_we_actually_got)
{
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, free_buffer(package2.buffer_id()));
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package);
    vault.wire_transfer_inbound(package2);
}

TEST_F(BufferVault, creates_buffer_on_first_insertion)
{
    EXPECT_CALL(mock_factory, create_buffer(_,initial_properties.size,initial_properties.format));
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package);
}

TEST_F(BufferVault, updates_buffer_on_subsequent_insertions)
{
    auto mock_buffer = std::make_shared<NiceMock<MockBuffer>>(size);
    EXPECT_CALL(*mock_buffer, update_from(_));
    ON_CALL(mock_factory, create_buffer(_,_,_))
        .WillByDefault(Return(mock_buffer));

    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package);
    auto b = vault.withdraw().get();
    vault.deposit(b);
    vault.wire_transfer_outbound(b);
    vault.wire_transfer_inbound(package);
}

TEST_F(BufferVault, withdrawing_and_never_filling_up_will_timeout)
{
    using namespace std::literals::chrono_literals;

    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
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
    EXPECT_CALL(mock_requests, submit_buffer(Ref(*buffer)));
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
    auto buffer = std::make_shared<MockBuffer>(size); 
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
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);

    EXPECT_CALL(mock_factory, create_buffer(_,initial_properties.size,initial_properties.format))
        .Times(Exactly(1));
    vault.wire_transfer_inbound(package);
    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);

    //should just activate, not create the buffer
    vault.wire_transfer_inbound(package);
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
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);

    auto f_buffer1 = vault.withdraw();
    auto f_buffer2 = vault.withdraw();
    vault.wire_transfer_inbound(package);
    vault.wire_transfer_inbound(package2);

    auto buffer1 = f_buffer1.get();
    auto buffer2 = f_buffer2.get();
    EXPECT_THAT(buffer1, Ne(buffer2));
}

TEST_F(BufferVault, destruction_signals_futures)
{
    using namespace std::literals::chrono_literals;
    mcl::NoTLSFuture<std::shared_ptr<mcl::ClientBuffer>> fbuffer;
    {
        mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
            size, format, usage, initial_nbuffers);
        fbuffer = vault.withdraw();
    }
    EXPECT_THAT(fbuffer.wait_for(5s), Ne(std::future_status::timeout));
}

TEST_F(BufferVault, ages_buffer_on_deposit)
{
    auto mock_buffer = std::make_shared<NiceMock<MockBuffer>>(size);
    EXPECT_CALL(*mock_buffer, increment_age());
    ON_CALL(mock_factory, create_buffer(_,_,_))
        .WillByDefault(Return(mock_buffer));

    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package);
    vault.deposit(vault.withdraw().get());
}

TEST_F(BufferVault, marks_as_submitted_on_transfer)
{
    auto mock_buffer = std::make_shared<NiceMock<MockBuffer>>(size);
    EXPECT_CALL(*mock_buffer, mark_as_submitted());
    ON_CALL(mock_factory, create_buffer(_,_,_))
        .WillByDefault(Return(mock_buffer));

    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        size, format, usage, initial_nbuffers);
    vault.wire_transfer_inbound(package);

    auto buffer = vault.withdraw().get();
    vault.deposit(buffer);
    vault.wire_transfer_outbound(buffer);
}

//handy for android's cancelbuffer
TEST_F(StartedBufferVault, can_withdraw_and_deposit)
{
    auto a_few_times = 5u;
    std::vector<std::shared_ptr<mcl::ClientBuffer>> buffers(a_few_times);
    for (auto i = 0u; i < a_few_times; i++)
    {
        buffers[i] = vault.withdraw().get();
        vault.deposit(buffers[i]);
    }
    EXPECT_THAT(buffers, Each(buffers[0]));
}


TEST_F(StartedBufferVault, reallocates_incoming_buffers_of_incorrect_size_with_immediate_response)
{
    mp::Buffer package4;
    geom::Size new_size{80, 100}; 
    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_))
        .WillOnce(Invoke(
        [&, this](geom::Size sz, MirPixelFormat, int)
        {
            package4.set_width(sz.width.as_int());
            package4.set_height(sz.height.as_int());
            package4.set_buffer_id(4);
            vault.wire_transfer_inbound(package4);
        }));

    vault.set_size(new_size);
    vault.wire_transfer_inbound(package);
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, reallocates_incoming_buffers_of_incorrect_size_with_delayed_response)
{
    geom::Size new_size{80, 100}; 
    mp::Buffer package4;
    package4.set_width(new_size.width.as_int());
    package4.set_height(new_size.height.as_int());
    package4.set_buffer_id(4);

    EXPECT_CALL(mock_requests, free_buffer(package.buffer_id()));
    EXPECT_CALL(mock_requests, allocate_buffer(new_size,_,_));

    vault.set_size(new_size);
    vault.wire_transfer_inbound(package);
    vault.wire_transfer_inbound(package4);
    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, withdraw_gives_only_newly_sized_buffers_after_resize)
{
    mp::Buffer package4;
    geom::Size new_size{80, 100};
    package4.set_width(new_size.width.as_int());
    package4.set_height(new_size.height.as_int());
    package4.set_buffer_id(4);
 
    vault.set_size(new_size);
    vault.wire_transfer_inbound(package4);

    EXPECT_THAT(vault.withdraw().get()->size(), Eq(new_size));
    Mock::VerifyAndClearExpectations(&mock_requests);
}

TEST_F(StartedBufferVault, simply_setting_size_triggers_no_server_interations)
{
    EXPECT_CALL(mock_requests, free_buffer(_)).Times(0);
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
