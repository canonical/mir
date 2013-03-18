/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "src/client/client_buffer_depository.h"
#include "src/client/client_buffer_factory.h"
#include "src/client/client_buffer.h"
#include "mir/geometry/pixel_format.h"
#include "mir/geometry/size.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom=mir::geometry;
namespace mcl=mir::client;

struct MockBuffer : public mcl::ClientBuffer
{
    MockBuffer()
    {
        using namespace testing;

        internal_age = 0;

        ON_CALL(*this, increment_age())
            .WillByDefault(InvokeWithoutArgs([this](){++this->internal_age;}));
        ON_CALL(*this, mark_as_submitted())
            .WillByDefault(Assign(&internal_age, 1));
        ON_CALL(*this, age())
            .WillByDefault(ReturnPointee(&internal_age));
    }

    MOCK_METHOD0(Destroy, void());
    virtual ~MockBuffer()
    {
        Destroy();
    }

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());
    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(increment_age, void());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_CONST_METHOD0(get_buffer_package, std::shared_ptr<MirBufferPackage>());
    MOCK_METHOD0(get_native_handle, MirNativeBuffer());

    int internal_age;
};

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
    {
        using namespace testing;

        ON_CALL(*this, create_buffer_rv(_,_,_))
            .WillByDefault(ReturnPointee(&buffer));
    }

    std::shared_ptr<mcl::ClientBuffer> create_buffer(std::shared_ptr<MirBufferPackage> && p,
                                                     geom::Size size, geom::PixelFormat pf)
    {
        buffer = std::make_shared<testing::NiceMock<MockBuffer>>();
        return create_buffer_rv( p, size, pf);
    }

    MOCK_METHOD3(create_buffer_rv,
                 std::shared_ptr<mcl::ClientBuffer>(std::shared_ptr<MirBufferPackage>,
                                                    geom::Size, geom::PixelFormat));

    std::shared_ptr<mcl::ClientBuffer> buffer;
};

struct MirBufferDepositoryTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{width, height};

        package = std::make_shared<MirBufferPackage>();
        mock_factory = std::make_shared<testing::NiceMock<MockClientBufferFactory>>();
    }
    geom::Width width;
    geom::Height height;
    geom::PixelFormat pf;
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

    EXPECT_CALL(*mock_factory, create_buffer_rv(_,size,pf))
        .Times(1);
    depository.deposit_package(std::move(package), 8, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_new_deposit_changes_current_buffer)
{
    using namespace testing;

    auto package2 = std::make_shared<MirBufferPackage>();
    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(std::move(package), 8, size, pf);
    auto buffer1 = depository.access_current_buffer();

    depository.deposit_package(std::move(package2), 9, size, pf);
    auto buffer2 = depository.access_current_buffer();

    EXPECT_NE(buffer1, buffer2);
}

TEST_F(MirBufferDepositoryTest, depository_sets_buffer_age_to_zero_for_new_buffer)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(std::move(package), 1, size, pf);
    auto buffer1 = depository.access_current_buffer();

    EXPECT_EQ(0u, buffer1->age());
}

TEST_F(MirBufferDepositoryTest, just_sumbitted_buffer_has_age_1)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};
    auto package2 = std::make_shared<MirBufferPackage>();

    depository.deposit_package(std::move(package), 1, size, pf);
    auto buffer1 = depository.access_current_buffer();

    ASSERT_EQ(0u, buffer1->age());

    // Deposit new package, implicitly marking previous buffer as submitted
    depository.deposit_package(std::move(package2), 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
}

TEST_F(MirBufferDepositoryTest, submitting_buffer_ages_other_buffers)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(std::move(package), 1, size, pf);
    auto buffer1 = depository.access_current_buffer();

    EXPECT_EQ(0u, buffer1->age());

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(std::move(package2), 2, size, pf);
    auto buffer2 = depository.access_current_buffer();

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(0u, buffer2->age());

    auto package3 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(std::move(package3), 3, size, pf);
    auto buffer3 = depository.access_current_buffer();

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());
    EXPECT_EQ(0u, buffer3->age());
}

TEST_F(MirBufferDepositoryTest, double_buffering_reaches_steady_state_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(std::move(package), 1, size, pf);
    auto buffer1 = depository.access_current_buffer();

    EXPECT_EQ(0u, buffer1->age());

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(std::move(package2), 2, size, pf);
    auto buffer2 = depository.access_current_buffer();

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(0u, buffer2->age());

    depository.deposit_package(std::move(package), 1, size, pf);

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());

    depository.deposit_package(std::move(package2), 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(2u, buffer2->age());
}

TEST_F(MirBufferDepositoryTest, triple_buffering_reaches_steady_state_age)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(std::move(package), 1, size, pf);
    auto buffer1 = depository.access_current_buffer();

    auto package2 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(std::move(package2), 2, size, pf);
    auto buffer2 = depository.access_current_buffer();

    auto package3 = std::make_shared<MirBufferPackage>();
    depository.deposit_package(std::move(package3), 3, size, pf);
    auto buffer3 = depository.access_current_buffer();

    EXPECT_EQ(2u, buffer1->age());
    EXPECT_EQ(1u, buffer2->age());
    EXPECT_EQ(0u, buffer3->age());

    depository.deposit_package(std::move(package), 1, size, pf);

    EXPECT_EQ(3u, buffer1->age());
    EXPECT_EQ(2u, buffer2->age());
    EXPECT_EQ(1u, buffer3->age());

    depository.deposit_package(std::move(package2), 2, size, pf);

    EXPECT_EQ(1u, buffer1->age());
    EXPECT_EQ(3u, buffer2->age());
    EXPECT_EQ(2u, buffer3->age());

    depository.deposit_package(std::move(package3), 3, size, pf);

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

    depository.deposit_package(std::move(packages[0]), 1, size, pf);
    MockBuffer* first_buffer = reinterpret_cast<MockBuffer *>(depository.access_current_buffer().get());

    depository.deposit_package(std::move(packages[1]), 2, size, pf);
    depository.deposit_package(std::move(packages[2]), 3, size, pf);

    // We've deposited three different buffers now; the fourth should trigger the destruction
    // of the first buffer.
    EXPECT_CALL(*first_buffer, Destroy()).Times(1);
    depository.deposit_package(std::move(packages[3]), 4, size, pf);
}

TEST_F(MirBufferDepositoryTest, depositing_packages_implicitly_submits_current_buffer)
{
    using namespace testing;
    const int num_buffers = 3;

    mcl::ClientBufferDepository depository{mock_factory, num_buffers};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();

    depository.deposit_package(std::move(package1), 1, size, pf);
    EXPECT_CALL(*reinterpret_cast<MockBuffer *>(depository.access_current_buffer().get()), mark_as_submitted());
    depository.deposit_package(std::move(package2), 2, size, pf);
}
