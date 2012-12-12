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

#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_swapper.h"

#include "mir_test_doubles/mock_swapper.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test/gmock_fixes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct MockIDGenerator: public mc::BufferIDUniqueGenerator
{
    MockIDGenerator()
    {
        using namespace testing;
        id = mc::BufferID{34};
        ON_CALL(*this, generate_unique_id())
            .WillByDefault(Return(id));
    }

    MOCK_METHOD0(generate_unique_id, mc::BufferID());

    mc::BufferID id;
};

class BufferBundleTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        size = geom::Size{geom::Width{1024}, geom::Height{768}};
        stride = geom::Stride{1024};
        pixel_format = geom::PixelFormat{geom::PixelFormat::rgba_8888};

        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(size, stride, pixel_format);
        second_mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(size, stride, pixel_format);
        third_mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(size, stride, pixel_format);
        mock_swapper = std::unique_ptr<NiceMock<mtd::MockSwapper>>(
            new NiceMock<mtd::MockSwapper>(mock_buffer));
        
        mock_generator = std::make_shared<NiceMock<MockIDGenerator>>();
    }

    std::shared_ptr<testing::NiceMock<mtd::MockBuffer>> mock_buffer;
    std::shared_ptr<testing::NiceMock<mtd::MockBuffer>> second_mock_buffer;
    std::shared_ptr<testing::NiceMock<mtd::MockBuffer>> third_mock_buffer;
    std::unique_ptr<testing::NiceMock<mtd::MockSwapper>> mock_swapper;
    geom::Size size;
    geom::Stride stride;
    geom::PixelFormat pixel_format;
    std::shared_ptr<testing::NiceMock<MockIDGenerator>> mock_generator;
};

TEST_F(BufferBundleTest, get_buffer_for_compositor_handles_resources)
{
    using namespace testing;

    EXPECT_CALL(*mock_swapper, compositor_acquire(_,_))
    .Times(1);
    EXPECT_CALL(*mock_swapper, compositor_release(_))
    .Times(1);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);

    auto texture = buffer_bundle.lock_back_buffer();
}

TEST_F(BufferBundleTest, get_buffer_for_client_releases_resources)
{
    using namespace testing;

    EXPECT_CALL(*mock_swapper, client_acquire(_,_))
    .Times(1);
    EXPECT_CALL(*mock_swapper, client_release(_))
    .Times(1);
    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);

    auto buffer_resource = buffer_bundle.secure_client_buffer();
}

TEST_F(BufferBundleTest, client_requesting_resource_queries_for_ipc_package)
{
    EXPECT_CALL(*mock_buffer, get_ipc_package())
    .Times(1);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);
    auto buffer_package = buffer_bundle.secure_client_buffer();
}

TEST_F(BufferBundleTest, client_requesting_package_gets_buffers_package)
{
    using namespace testing;

    std::shared_ptr<mc::BufferIPCPackage> dummy_ipc_package = std::make_shared<mc::BufferIPCPackage>();
    EXPECT_CALL(*mock_buffer, get_ipc_package())
    .Times(1)
    .WillOnce(Return(dummy_ipc_package));
    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);

    std::shared_ptr<mc::GraphicBufferClientResource> buffer_resource = buffer_bundle.secure_client_buffer();
    auto buffer_package = buffer_resource->buffer.lock()->get_ipc_package();
    EXPECT_EQ(buffer_package, dummy_ipc_package);
}

#if 0
TEST_F(BufferBundleTest, new_buffer_from_swapper_generates_new_id_once_with_same_buffer)
{
    using namespace testing;

    int num_iteration = 5;
    EXPECT_CALL(*mock_swapper, client_acquire(_,_))
        .Times(num_iteration)
        .WillRepeatedly(Return(mock_buffer.get())); 
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(1);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);
    for(auto i=0; i<num_iteration; i++)
    {
        auto buffer_resource = buffer_bundle.secure_client_buffer();
    }
}

TEST_F(BufferBundleTest, new_buffer_from_swapper_generates_new_id_thrice_with_three_different_buffers)
{
    using namespace testing;

    int num_iteration = 7;
    EXPECT_CALL(*mock_swapper, client_acquire(_,_))
        .Times(num_iteration)
        .WillOnce(Return(mock_buffer.get()))
        .WillOnce(Return(second_mock_buffer.get()))
        .WillRepeatedly(Return(third_mock_buffer.get()));
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(3);

    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);
 
    for(auto i=0; i<num_iteration; i++)
    {
        buffer_bundle.secure_client_buffer();
    }
}

TEST_F(BufferBundleTest, client_requesting_package_gets_buffers_package_with_valid_id)
{
    using namespace testing;
    std::shared_ptr<mc::BufferIPCPackage> dummy_ipc_package = std::make_shared<mc::BufferIPCPackage>();
    EXPECT_CALL(*mock_buffer, get_ipc_package())
    .Times(1)
    .WillOnce(Return(dummy_ipc_package));
    mc::BufferBundleSurfaces buffer_bundle(std::move(mock_swapper), mock_generator);

    std::shared_ptr<mc::GraphicBufferClientResource> buffer_resource = buffer_bundle.secure_client_buffer();   
    EXPECT_TRUE(buffer_resource->id.is_valid()); 
    EXPECT_EQ(buffer_resource->id, mock_generator->id); 
}
#endif
