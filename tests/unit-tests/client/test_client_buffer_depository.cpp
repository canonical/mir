/*
 * Copyright © 2013 Canonical Ltd.
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
#include "src/client/client_buffer_factory.h"
#include "src/client/aging_buffer.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom=mir::geometry;
namespace mcl=mir::client;

struct MockBuffer : public mcl::AgingBuffer
{
    MockBuffer()
    {
        using namespace testing;
        ON_CALL(*this, mark_as_submitted())
            .WillByDefault(Invoke([this](){this->AgingBuffer::mark_as_submitted();}));

        // By default we expect all buffers to be destroyed.
        EXPECT_CALL(*this, Destroy()).Times(1);
    }

    MOCK_METHOD0(Destroy, void());
    virtual ~MockBuffer() noexcept
    {
        Destroy();
    }

    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, MirPixelFormat());
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<mir::graphics::NativeBuffer>());
};

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
    {
        using namespace testing;

        // Some tests, quite reasonably, rely on create_buffer returning a different buffer each time
        // Handle this by first updating our "buffer" temporary, then returning-by-pointee.
        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(DoAll(InvokeWithoutArgs([this]() {this->buffer = std::make_shared<NiceMock<MockBuffer>>();}),
                                 ReturnPointee(&buffer)));
    }

    MOCK_METHOD3(create_buffer,
                 std::shared_ptr<mcl::ClientBuffer>(std::shared_ptr<MirBufferPackage> const&,
                                                    geom::Size, MirPixelFormat));

    std::shared_ptr<mcl::ClientBuffer> buffer;
};

struct MirBufferDepositoryTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{width, height};

        package = std::make_shared<MirBufferPackage>();
        mock_factory = std::make_shared<testing::NiceMock<MockClientBufferFactory>>();
    }
    geom::Width width;
    geom::Height height;
    MirPixelFormat pf;
    geom::Size size;

    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MockClientBufferFactory> mock_factory;
};

MATCHER_P(SizeMatches, value, "")
{
    return value == arg;
}

TEST_F(MirBufferDepositoryTest, depository_sets_width_and_height)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    EXPECT_CALL(*mock_factory, create_buffer(_,size,pf))
        .Times(1);
    depository.deposit_package(package, 8, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_new_deposit_changes_current_buffer)
{
    using namespace testing;

    auto package2 = std::make_shared<MirBufferPackage>();
    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 8, size, pf);
    auto buffer1 = depository.current_buffer();

    depository.deposit_package(package2, 9, size, pf);
    auto buffer2 = depository.current_buffer();

    EXPECT_NE(buffer1, buffer2);
}

TEST_F(MirBufferDepositoryTest, depository_sets_buffer_age_to_zero_for_new_buffer)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    EXPECT_EQ(0u, buffer1->age());
}

TEST_F(MirBufferDepositoryTest, just_sumbitted_buffer_has_age_1)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};
    auto package2 = std::make_shared<MirBufferPackage>();

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    ASSERT_EQ(0u, buffer1->age());

    // Deposit new package, implicitly marking previous buffer as submitted
    depository.deposit_package(package2, 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
}

TEST_F(MirBufferDepositoryTest, submitting_buffer_ages_other_buffers)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    EXPECT_EQ(0u, buffer1->age());

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(package2, 2, size, pf);
    auto buffer2 = depository.current_buffer();

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(0u, buffer2->age());

    auto package3 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(package3, 3, size, pf);
    auto buffer3 = depository.current_buffer();

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());
    EXPECT_EQ(0u, buffer3->age());
}

TEST_F(MirBufferDepositoryTest, double_buffering_reaches_steady_state_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    EXPECT_EQ(0u, buffer1->age());

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(package2, 2, size, pf);
    auto buffer2 = depository.current_buffer();

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(0u, buffer2->age());

    depository.deposit_package(package, 1, size, pf);

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());

    depository.deposit_package(package2, 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(2u, buffer2->age());
}

TEST_F(MirBufferDepositoryTest, triple_buffering_reaches_steady_state_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(package2, 2, size, pf);
    auto buffer2 = depository.current_buffer();

    auto package3 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(package3, 3, size, pf);
    auto buffer3 = depository.current_buffer();

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());
    EXPECT_EQ(0u, buffer3->age());

    depository.deposit_package(package, 1, size, pf);

    EXPECT_EQ(3u, buffer1->age());
    EXPECT_EQ(2u, buffer2->age());
    EXPECT_EQ(1u, buffer3->age());

    depository.deposit_package(package2, 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(3u, buffer2->age());
    EXPECT_EQ(2u, buffer3->age());

    depository.deposit_package(package3, 3, size, pf);

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());
    EXPECT_EQ(3u, buffer3->age());
}

TEST_F(MirBufferDepositoryTest, depository_destroys_old_buffers)
{
    using namespace testing;
    const int num_buffers = 3;

    mcl::ClientBufferDepository depository{mock_factory, num_buffers};

    const int num_packages = 4;
    std::shared_ptr<MirBufferPackage> packages[num_packages];

    depository.deposit_package(packages[0], 1, size, pf);
    // Raw pointer so we don't influence the buffer's life-cycle
    MockBuffer *first_buffer = static_cast<MockBuffer *>(depository.current_buffer().get());
    // We expect this to not be destroyed before we deposit the fourth buffer.
    bool buffer_destroyed = false;
    ON_CALL(*first_buffer, Destroy()).WillByDefault(Invoke([&buffer_destroyed] () {buffer_destroyed = true;}));


    depository.deposit_package(packages[1], 2, size, pf);
    depository.deposit_package(packages[2], 3, size, pf);

    // We've deposited three different buffers now; the fourth should trigger the destruction
    // of the first buffer.
    ASSERT_FALSE(buffer_destroyed);

    depository.deposit_package(packages[3], 4, size, pf);

    // Explicitly verify that the buffer has been destroyed here, before the depository goes out of scope
    // and its destructor cleans everything up.
    EXPECT_TRUE(buffer_destroyed);
}

TEST_F(MirBufferDepositoryTest, depositing_packages_implicitly_submits_current_buffer)
{
    using namespace testing;
    const int num_buffers = 3;

    mcl::ClientBufferDepository depository{mock_factory, num_buffers};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();

    depository.deposit_package(package1, 1, size, pf);
    EXPECT_CALL(*static_cast<MockBuffer *>(depository.current_buffer().get()), mark_as_submitted());
    depository.deposit_package(package2, 2, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_respects_max_buffer_parameter)
{
    using namespace testing;
    std::shared_ptr<mcl::ClientBufferDepository> depository;
    std::shared_ptr<MirBufferPackage> packages[10];
    bool buffer_destroyed[10];

    for (int num_buffers = 2; num_buffers < 10; ++num_buffers)
    {
        depository = std::make_shared<mcl::ClientBufferDepository>(mock_factory, num_buffers);

        // Reset destroyed tracking; resetting the depository will have destroyed all the buffers
        for (bool& destroyed_flag : buffer_destroyed)
            destroyed_flag = false;

        int i;
        for (i = 0; i < num_buffers ; ++i)
        {
            MockBuffer *buffer;
            depository->deposit_package(packages[i], i + 1, size, pf);
            buffer = static_cast<MockBuffer *>(depository->current_buffer().get());
            ON_CALL(*buffer, Destroy()).WillByDefault(Invoke([&buffer_destroyed, i] () {buffer_destroyed[i] = true;}));
        }

        // Next deposit should destroy first buffer
        ASSERT_FALSE(buffer_destroyed[0]);
        depository->deposit_package(packages[i], i+1, size, pf);
        EXPECT_TRUE(buffer_destroyed[0]);

        // Verify none of the other buffers have been destroyed
        for (i = 1; i < num_buffers; ++i)
            EXPECT_FALSE(buffer_destroyed[i]);
    }
}

TEST_F(MirBufferDepositoryTest, depository_caches_recently_seen_buffer)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();

    EXPECT_CALL(*mock_factory, create_buffer(_,_,_))
        .Times(1);

    depository.deposit_package(package1, 8, size, pf);
    depository.deposit_package(package1, 8, size, pf);
    depository.deposit_package(package1, 8, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_creates_new_buffer_for_different_id)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();

    EXPECT_CALL(*mock_factory, create_buffer(_,_,_))
        .Times(2);

    depository.deposit_package(package1, 8, size, pf);
    depository.deposit_package(package2, 9, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_keeps_last_2_buffers_regardless_of_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 2};

    EXPECT_CALL(*mock_factory, create_buffer(_,_,_)).Times(2);

    depository.deposit_package(package, 8, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 8, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_keeps_last_3_buffers_regardless_of_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    EXPECT_CALL(*mock_factory, create_buffer(_,_,_)).Times(3);

    depository.deposit_package(package, 8, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.deposit_package(package, 8, size, pf);
}
