/*
 * Copyright © 2012 Canonical Ltd.
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
#include "src/client/android/android_client_buffer_depository.h"
#include "src/client/android/android_client_buffer.h"
#include "mir_test_doubles/mock_android_registrar.h"

#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace mcla = mir::client::android;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;


struct MirBufferDepositoryTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        size = geom::Size{width, height};
        pf = geom::PixelFormat::abgr_8888;

        mock_registrar = std::make_shared<mtd::MockAndroidRegistrar>();
        package1 = std::make_shared<MirBufferPackage>();
        package2 = std::make_shared<MirBufferPackage>();

    }
    geom::Width width;
    geom::Height height;
    geom::PixelFormat pf;
    geom::Size size;

    std::shared_ptr<mtd::MockAndroidRegistrar> mock_registrar;

    std::shared_ptr<MirBufferPackage> package1;
    std::shared_ptr<MirBufferPackage> package2;

};

TEST_F(MirBufferDepositoryTest, depository_sets_width_and_height)
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(1);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(1);

    depository.deposit_package(std::move(package1), 8, size, pf);
    auto buffer = depository.access_buffer(8);

    EXPECT_EQ(buffer->size().height, height);
    EXPECT_EQ(buffer->size().width, width);
    EXPECT_EQ(buffer->pixel_format(), pf);
}

TEST_F(MirBufferDepositoryTest, depository_does_not_create_a_buffer_its_seen_before )
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(1);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(1);

    /* repeated id */
    depository.deposit_package(std::move(package1), 8, size, pf);
    depository.deposit_package(std::move(package2), 8, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_creates_two_buffers_with_distinct_id )
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(2);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(2);

    /* repeated id */
    depository.deposit_package(std::move(package1), 8, size, pf);
    depository.deposit_package(std::move(package2), 9, size, pf);
}

TEST_F(MirBufferDepositoryTest, depository_returns_same_accessed_buffer_for_same_id )
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(2);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(2);

    /* repeated id */
    depository.deposit_package(std::move(package1), 8, size, pf);
    depository.deposit_package(std::move(package2), 9, size, pf);

    auto buffer1 = depository.access_buffer(8);
    auto buffer2 = depository.access_buffer(8);

    EXPECT_EQ(buffer1, buffer2);
}

TEST_F(MirBufferDepositoryTest, depository_returns_different_accessed_buffer_for_unique_id )
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(2);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(2);

    /* repeated id */
    depository.deposit_package(std::move(package1), 8, size, pf);
    depository.deposit_package(std::move(package2), 9, size, pf);

    auto buffer1 = depository.access_buffer(8);
    auto buffer2 = depository.access_buffer(9);

    EXPECT_NE(buffer1, buffer2);
}

TEST_F(MirBufferDepositoryTest, depository_throws_for_uncreated_id )
{
    using namespace testing;

    mcla::AndroidClientBufferDepository depository(mock_registrar);

    EXPECT_CALL(*mock_registrar, register_buffer(_))
        .Times(1);
    EXPECT_CALL(*mock_registrar, unregister_buffer(_))
        .Times(1);

    /* repeated id */
    depository.deposit_package(std::move(package1), 8, size, pf);

    EXPECT_THROW({
    auto buffer2 = depository.access_buffer(9);
    }, std::runtime_error);

}
