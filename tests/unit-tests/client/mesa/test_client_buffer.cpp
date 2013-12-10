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
#include "src/client/mesa/client_buffer.h"
#include "src/client/mesa/client_buffer_factory.h"
#include "src/client/mesa/buffer_file_ops.h"

#include <sys/mman.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace geom=mir::geometry;
namespace mclg=mir::client::mesa;

namespace
{

class MockBufferFileOps : public mclg::BufferFileOps
{
public:
    MOCK_CONST_METHOD1(close, int(int fd));
    MOCK_CONST_METHOD3(map, void*(int fd, off_t offset, size_t size));
    MOCK_CONST_METHOD2(unmap, void(void* addr, size_t size));
};

struct MesaClientBufferTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        stride = geom::Stride(66);
        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{width, height};
        buffer_file_ops = std::make_shared<testing::NiceMock<MockBufferFileOps>>();

        package = std::make_shared<MirBufferPackage>();
        package->fd[0] = 167;
        package->fd_items = 1;
        package->stride = stride.as_uint32_t();
        package->width = width.as_int();
        package->height = height.as_int();

        package_copy = std::make_shared<MirBufferPackage>(*package.get());
    }
    geom::Width width;
    geom::Height height;
    geom::Stride stride;
    MirPixelFormat pf;
    geom::Size size;

    std::shared_ptr<testing::NiceMock<MockBufferFileOps>> buffer_file_ops;
    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MirBufferPackage> package_copy;

};

}

TEST_F(MesaClientBufferTest, width_and_height)
{
    using namespace testing;

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    EXPECT_EQ(buffer.size().height, height);
    EXPECT_EQ(buffer.size().width, width);
    EXPECT_EQ(buffer.pixel_format(), pf);
}

TEST_F(MesaClientBufferTest, buffer_returns_correct_stride)
{
    using namespace testing;

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    EXPECT_EQ(buffer.stride(), stride);
}

TEST_F(MesaClientBufferTest, buffer_returns_set_package)
{
    using namespace testing;

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    auto package_return = buffer.native_buffer_handle();
    EXPECT_EQ(package_return->data_items, package_copy->data_items);
    EXPECT_EQ(package_return->fd_items, package_copy->fd_items);
    EXPECT_EQ(package_return->stride, package_copy->stride);
    for (auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->data[i], package_copy->data[i]);
    for (auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->fd[i], package_copy->fd[i]);
}

TEST_F(MesaClientBufferTest, secure_for_cpu_write_maps_buffer_fd)
{
    using namespace testing;
    void *map_addr{reinterpret_cast<void*>(0xabcdef)};

    EXPECT_CALL(*buffer_file_ops, map(package->fd[0],_,_))
        .WillOnce(Return(map_addr));
    EXPECT_CALL(*buffer_file_ops, unmap(map_addr,_))
        .Times(1);
    EXPECT_CALL(*buffer_file_ops, close(package->fd[0]))
        .Times(1);

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    auto mem_region = buffer.secure_for_cpu_write();
    ASSERT_EQ(map_addr, mem_region->vaddr.get());
    ASSERT_EQ(width, mem_region->width);
    ASSERT_EQ(height, mem_region->height);
    ASSERT_EQ(stride, mem_region->stride);
    ASSERT_EQ(pf, mem_region->format);
}

TEST_F(MesaClientBufferTest, secure_for_cpu_write_throws_on_map_failure)
{
    using namespace testing;

    EXPECT_CALL(*buffer_file_ops, map(package->fd[0],_,_))
        .WillOnce(Return(MAP_FAILED));
    EXPECT_CALL(*buffer_file_ops, unmap(_,_))
        .Times(0);
    EXPECT_CALL(*buffer_file_ops, close(package->fd[0]))
        .Times(1);

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    EXPECT_THROW({
        auto mem_region = buffer.secure_for_cpu_write();
    }, std::runtime_error);
}

TEST_F(MesaClientBufferTest, buffer_fd_closed_on_buffer_destruction)
{
    using namespace testing;

    EXPECT_CALL(*buffer_file_ops, close(package->fd[0]))
        .Times(1);

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);
}

TEST_F(MesaClientBufferTest, factory_gets_size_from_package)
{
    using namespace testing;

    mclg::ClientBufferFactory factory(buffer_file_ops);

    geom::Size unused_size{0, 0};
    auto buffer = factory.create_buffer(package, unused_size, pf);

    auto const& buf_size = buffer->size();
    EXPECT_EQ(package->width, buf_size.width.as_int());
    EXPECT_EQ(package->height, buf_size.height.as_int());

    EXPECT_NE(unused_size, buf_size);
}
