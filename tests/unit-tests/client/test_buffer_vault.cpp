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

struct MockBuffer : public mcl::AgingBuffer
{
    MockBuffer()
    {
        using namespace testing;
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
};

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory() :
        alloc_count(0),
        free_count(0)
    {
        using namespace testing;
        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(InvokeWithoutArgs([this]()
            {
                alloc_count++;
                auto buffer = std::shared_ptr<mcl::ClientBuffer>(
                    new NiceMock<MockBuffer>(),
                    [this](mcl::ClientBuffer* buffer)
                    {
                        free_count++;
                        delete buffer;
                    });
                if (alloc_count == 1)
                    first_allocated_buffer = buffer;
                return buffer;
            }));
    }

    ~MockClientBufferFactory()
    {
        EXPECT_THAT(alloc_count, testing::Eq(free_count));
    }

    MOCK_METHOD3(create_buffer,
                 std::shared_ptr<mcl::ClientBuffer>(std::shared_ptr<MirBufferPackage> const&,
                                                    geom::Size, MirPixelFormat));

    bool first_buffer_allocated_and_then_freed()
    {
        if ((alloc_count >= 1) && (first_allocated_buffer.lock() == nullptr))
            return true;
        return false;
    }

    int alloc_count;
    int free_count;
    std::weak_ptr<mcl::ClientBuffer> first_allocated_buffer;
};


#include <future>

namespace mir
{
namespace client
{
class ServerBufferRequests
{
public:
    virtual void allocate_buffer() = 0;
    virtual void free_buffer() = 0;
    virtual void submit_buffer() = 0;
    virtual ~ServerBufferRequests() = default;
protected:
    ServerBufferRequests() = default;
    ServerBufferRequests(ServerBufferRequests const&) = delete;
    ServerBufferRequests& operator=(ServerBufferRequests const&) = delete;
};
}
}

struct MockServerRequests : mcl::ServerBufferRequests
{
    MOCK_METHOD0(allocate_buffer, void());
    MOCK_METHOD0(free_buffer, void());
    MOCK_METHOD0(submit_buffer, void());
};

struct BufferVault : public testing::Test
{
    unsigned int initial_nbuffers {3};
    mg::BufferProperties initial_properties{
        geom::Size{271,314}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    MockClientBufferFactory mock_factory;
    MockServerRequests mock_requests;
    mp::Buffer package;
};

struct StartedBufferVault : BufferVault
{
    StartedBufferVault()
    {
    }
    mcl::BufferVault vault{
        mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        initial_nbuffers, initial_properties};
};

TEST_F(BufferVault, creates_all_buffers_on_start)
{
    EXPECT_CALL(mock_requests, allocate_buffer())
        .Times(initial_nbuffers);
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        initial_nbuffers, initial_properties);
}

TEST_F(BufferVault, creates_buffer_on_first_insertion)
{
    EXPECT_CALL(mock_factory, create_buffer(_,initial_properties.size,initial_properties.format));
    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        initial_nbuffers, initial_properties);
    vault.wire_transfer_inbound(package);
}

TEST_F(BufferVault, withdrawing_and_never_filling_up_will_timeout)
{
    using namespace std::literals::chrono_literals;

    mcl::BufferVault vault(mt::fake_shared(mock_factory), mt::fake_shared(mock_requests),
        initial_nbuffers, initial_properties);
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
    EXPECT_CALL(mock_requests, submit_buffer());
    auto buffer = vault.withdraw().get();
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

#if 0
TEST_F(StartedBufferVault, ages_buffers_on_deposit)
{
    auto buffer = vault.withdraw();
    auto buffer_age = buffer->age(); 
    vault.deposit(buffer);
    EXPECT_THAT(buffer->age, Eq(buffer_age + 1));
}

TEST_F(StartedBufferVault, selects_first_given_buffer_if_its_got_a_surplus)
{
    auto buffer = vault.withdraw();
//    auto buffer = vault.withdraw();

    
}

TEST_F(BufferSlots, can_access_last_inserted_buffer)
{
    mcl::BufferVault vault(mock_factory, mock_requests);
    vault.insert(package);

    auto buffer = vault.access();
    EXPECT_THAT(buffer, NotNull());
}

TEST_F(BufferSlots, can_get_buffer_back)
{
    mcl::BufferVault vault(mock_factory, mock_requests);
    vault.insert(package);

    vault.mark_as_submitted(package->id);
    vault.insert(package);

}
#endif
