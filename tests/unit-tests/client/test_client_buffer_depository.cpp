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
#include "mir/client_buffer_factory.h"
#include "mir/aging_buffer.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_client_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace geom=mir::geometry;
namespace mcl=mir::client;
namespace mtd = mir::test::doubles;

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
                auto buffer = std::shared_ptr<mtd::MockClientBuffer>(
                    new NiceMock<mtd::MockClientBuffer>(),
                    [this](mcl::ClientBuffer* buffer)
                    {
                        free_count++;
                        delete buffer;
                    });
                std::weak_ptr<mtd::MockClientBuffer> wb (buffer);
                ON_CALL(*buffer, mark_as_submitted())
                    .WillByDefault(Invoke([wb]{ if (auto buffer = wb.lock()) buffer->age_ = 1; }));
                ON_CALL(*buffer, increment_age())
                    .WillByDefault(Invoke([wb]{ if (auto buffer = wb.lock()) if (buffer->age_) buffer->age_++; }));
                ON_CALL(*buffer, age())
                    .WillByDefault(Invoke([wb]{ if (auto buffer = wb.lock()) return buffer->age_; return 0; }));

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

struct ClientBufferDepository : public testing::Test
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

TEST_F(ClientBufferDepository, sets_width_and_height)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    EXPECT_CALL(*mock_factory, create_buffer(_,size,pf))
        .Times(1);
    depository.deposit_package(package, 8, size, pf);
}

TEST_F(ClientBufferDepository, changes_current_buffer_on_new_deposit)
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

TEST_F(ClientBufferDepository, sets_buffer_age_to_zero_for_new_buffer)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    depository.deposit_package(package, 1, size, pf);
    auto buffer1 = depository.current_buffer();

    EXPECT_EQ(0u, buffer1->age());
}

TEST_F(ClientBufferDepository, has_age_1_for_just_sumbitted_buffer)
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

TEST_F(ClientBufferDepository, ages_other_buffers_on_new_submission)
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

TEST_F(ClientBufferDepository, reaches_steady_state_age_for_double_buffering)
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

TEST_F(ClientBufferDepository, reaches_steady_state_age_when_triple_buffering)
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

TEST_F(ClientBufferDepository, destroys_old_buffers)
{
    using namespace testing;
    const int num_buffers = 3;

    mcl::ClientBufferDepository depository{mock_factory, num_buffers};

    const int num_packages = 4;
    std::shared_ptr<MirBufferPackage> packages[num_packages];

    depository.deposit_package(packages[0], 1, size, pf);
    depository.deposit_package(packages[1], 2, size, pf);
    depository.deposit_package(packages[2], 3, size, pf);

    // We've deposited three different buffers now; the fourth should trigger the destruction
    // of the first buffer.
    EXPECT_THAT(mock_factory->free_count, Eq(0));
    depository.deposit_package(packages[3], 4, size, pf);
    EXPECT_THAT(mock_factory->free_count, Eq(1));
    EXPECT_TRUE(mock_factory->first_buffer_allocated_and_then_freed());
}

TEST_F(ClientBufferDepository, implicitly_submits_current_buffer_on_deposit)
{
    using namespace testing;
    const int num_buffers = 3;

    mcl::ClientBufferDepository depository{mock_factory, num_buffers};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();

    depository.deposit_package(package1, 1, size, pf);
    EXPECT_CALL(*static_cast<mtd::MockClientBuffer*>(depository.current_buffer().get()), mark_as_submitted());
    depository.deposit_package(package2, 2, size, pf);
}

TEST_F(ClientBufferDepository, frees_buffers_after_reaching_capacity)
{
    using namespace testing;
    std::shared_ptr<mcl::ClientBufferDepository> depository;
    std::shared_ptr<MirBufferPackage> packages[10];

    for (int num_buffers = 2; num_buffers < 10; ++num_buffers)
    {
        depository = std::make_shared<mcl::ClientBufferDepository>(mock_factory, num_buffers);
        mock_factory->free_count = 0;
        mock_factory->alloc_count = 0;

        int i;
        for (i = 0; i < num_buffers ; ++i)
            depository->deposit_package(packages[i], i + 1, size, pf);

        // Next deposit should destroy a buffer
        EXPECT_THAT(mock_factory->free_count, Eq(0));
        depository->deposit_package(packages[i], i+1, size, pf);
        EXPECT_THAT(mock_factory->free_count, Eq(1));
        EXPECT_TRUE(mock_factory->first_buffer_allocated_and_then_freed());
    }
}

TEST_F(ClientBufferDepository, caches_recently_seen_buffer)
{
    using namespace testing;

    mcl::ClientBufferDepository depository{mock_factory, 3};

    auto package1 = std::make_shared<MirBufferPackage>();
    auto package2 = std::make_shared<MirBufferPackage>();
    auto package3 = std::make_shared<MirBufferPackage>();
    NiceMock<mtd::MockClientBuffer> mock_buffer;
    Sequence seq;
    EXPECT_CALL(*mock_factory, create_buffer(Ref(package1),_,_))
        .InSequence(seq)
        .WillOnce(Return(mir::test::fake_shared(mock_buffer)));
    EXPECT_CALL(mock_buffer, update_from(Ref(*package2)))
        .InSequence(seq);
    EXPECT_CALL(mock_buffer, update_from(Ref(*package3)))
        .InSequence(seq);

    depository.deposit_package(package1, 8, size, pf);
    depository.deposit_package(package2, 8, size, pf);
    depository.deposit_package(package3, 8, size, pf);
}

TEST_F(ClientBufferDepository, creates_new_buffer_for_different_id)
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

TEST_F(ClientBufferDepository, keeps_last_2_buffers_regardless_of_age)
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

TEST_F(ClientBufferDepository, keeps_last_3_buffers_regardless_of_age)
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

TEST_F(ClientBufferDepository, can_decrease_cache_size)
{
    using namespace testing;
    int initial_size{3};
    int changed_size{2};
    mcl::ClientBufferDepository depository{mock_factory, initial_size};
    EXPECT_CALL(*mock_factory, create_buffer(_,_,_))
        .Times(4);

    depository.deposit_package(package, 8, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);

    depository.set_max_buffers(changed_size); //8 should be kicked out
    depository.deposit_package(package, 8, size, pf);
}

TEST_F(ClientBufferDepository, can_increase_cache_size)
{
    using namespace testing;
    int initial_size{3};
    int changed_size{4};
    mcl::ClientBufferDepository depository{mock_factory, initial_size};
    EXPECT_CALL(*mock_factory, create_buffer(_,_,_))
        .Times(4);

    depository.deposit_package(package, 8, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.deposit_package(package, 9, size, pf);
    depository.deposit_package(package, 10, size, pf);
    depository.set_max_buffers(changed_size);
    depository.deposit_package(package, 7, size, pf);
    depository.deposit_package(package, 8, size, pf);
}

TEST_F(ClientBufferDepository, cannot_have_zero_size)
{
    int initial_size{3};
    EXPECT_THROW({
        mcl::ClientBufferDepository depository(mock_factory, 0);
    }, std::logic_error);

    mcl::ClientBufferDepository depository{mock_factory, initial_size};
    EXPECT_THROW({
        depository.set_max_buffers(0);
    }, std::logic_error);
}
