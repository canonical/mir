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
#include "src/client/gbm/gbm_client_buffer.h"
#include "src/client/gbm/gbm_client_buffer_factory.h"
#include "mock_drm_fd_handler.h"

#include <xf86drm.h>
#include <sys/mman.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace geom=mir::geometry;
namespace mclg=mir::client::gbm;

struct MirGBMBufferTest : public testing::Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        stride = geom::Stride(66);
        pf = geom::PixelFormat::abgr_8888;
        size = geom::Size{width, height};
        drm_fd_handler = std::make_shared<testing::NiceMock<mclg::MockDRMFDHandler>>();

        package = std::make_shared<MirBufferPackage>();
        package->stride = stride.as_uint32_t();
        package->width = width.as_int();
        package->height = height.as_int();

        package_copy = std::make_shared<MirBufferPackage>(*package.get());
    }
    geom::Width width;
    geom::Height height;
    geom::Stride stride;
    geom::PixelFormat pf;
    geom::Size size;

    std::shared_ptr<testing::NiceMock<mclg::MockDRMFDHandler>> drm_fd_handler;
    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MirBufferPackage> package_copy;

};

TEST_F(MirGBMBufferTest, width_and_height)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    EXPECT_EQ(buffer.size().height, height);
    EXPECT_EQ(buffer.size().width, width);
    EXPECT_EQ(buffer.pixel_format(), pf);
}

TEST_F(MirGBMBufferTest, buffer_returns_correct_stride)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    EXPECT_EQ(buffer.stride(), stride);
}

TEST_F(MirGBMBufferTest, buffer_returns_set_package)
{
    using namespace testing;

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    auto package_return = buffer.native_buffer_handle();
    EXPECT_EQ(package_return->data_items, package_copy->data_items);
    EXPECT_EQ(package_return->fd_items, package_copy->fd_items);
    EXPECT_EQ(package_return->stride, package_copy->stride);
    for (auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->data[i], package_copy->data[i]);
    for (auto i=0; i<mir_buffer_package_max; i++)
        EXPECT_EQ(package_return->fd[i], package_copy->fd[i]);
}

TEST_F(MirGBMBufferTest, secure_for_cpu_write_maps_drm_buffer)
{
    using namespace testing;
    void *map_addr{reinterpret_cast<void*>(0xabcdef)};

    EXPECT_CALL(*drm_fd_handler, primeFDToHandle(_,_))
        .WillOnce(Return(0));
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_MODE_MAP_DUMB,_))
        .WillOnce(Return(0));
    EXPECT_CALL(*drm_fd_handler, map(_,_))
        .WillOnce(Return(map_addr));
    EXPECT_CALL(*drm_fd_handler, unmap(_,_))
        .Times(1);
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_GEM_CLOSE,_))
        .Times(1);

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    auto mem_region = buffer.secure_for_cpu_write();
    ASSERT_EQ(map_addr, mem_region->vaddr.get());
    ASSERT_EQ(width, mem_region->width);
    ASSERT_EQ(height, mem_region->height);
    ASSERT_EQ(stride, mem_region->stride);
    ASSERT_EQ(pf, mem_region->format);
}

TEST_F(MirGBMBufferTest, secure_for_cpu_write_throws_on_prime_handle_failure)
{
    using namespace testing;

    EXPECT_CALL(*drm_fd_handler, primeFDToHandle(_,_))
        .WillOnce(Return(-1));
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_MODE_MAP_DUMB,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, map(_,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, unmap(_,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_GEM_CLOSE,_))
        .Times(0);

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    EXPECT_THROW({
        buffer.secure_for_cpu_write();
    }, std::runtime_error);
}

TEST_F(MirGBMBufferTest, secure_for_cpu_write_throws_on_map_dumb_failure)
{
    using namespace testing;

    EXPECT_CALL(*drm_fd_handler, primeFDToHandle(_,_))
        .WillOnce(Return(0));
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_MODE_MAP_DUMB,_))
        .WillOnce(Return(1));
    EXPECT_CALL(*drm_fd_handler, map(_,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, unmap(_,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_GEM_CLOSE,_))
        .Times(1);

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    EXPECT_THROW({
        auto mem_region = buffer.secure_for_cpu_write();
    }, std::runtime_error);
}

TEST_F(MirGBMBufferTest, secure_for_cpu_write_throws_on_map_failure)
{
    using namespace testing;

    EXPECT_CALL(*drm_fd_handler, primeFDToHandle(_,_))
        .WillOnce(Return(0));
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_MODE_MAP_DUMB,_))
        .WillOnce(Return(0));
    EXPECT_CALL(*drm_fd_handler, map(_,_))
        .WillOnce(Return(MAP_FAILED));
    EXPECT_CALL(*drm_fd_handler, unmap(_,_))
        .Times(0);
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_GEM_CLOSE,_))
        .Times(1);

    mclg::GBMClientBuffer buffer(drm_fd_handler, std::move(package), size, pf);

    EXPECT_THROW({
        auto mem_region = buffer.secure_for_cpu_write();
    }, std::runtime_error);
}

TEST_F(MirGBMBufferTest, prime_fd_closed_on_buffer_destruction)
{
    using namespace testing;

    int const prime_fd{42};

    package->fd[0] = prime_fd;
    package->fd_items = 1;

    EXPECT_CALL(*drm_fd_handler, close(prime_fd))
        .Times(1);

    mclg::GBMClientBuffer buffer(drm_fd_handler, package, size, pf);
}

TEST_F(MirGBMBufferTest, buffer_does_not_take_a_gem_reference_when_not_mapping)
{
    using namespace testing;

    // We don't map the buffer, so we don't need to take a GEM reference...
    EXPECT_CALL(*drm_fd_handler, primeFDToHandle(_,_))
        .Times(0);
    // We haven't taken a GEM reference, so we shouldn't close it.
    EXPECT_CALL(*drm_fd_handler, ioctl(DRM_IOCTL_GEM_CLOSE,_))
        .Times(0);

    mclg::GBMClientBuffer buffer(drm_fd_handler, package, size, pf);
}

TEST_F(MirGBMBufferTest, factory_gets_size_from_package)
{
    using namespace testing;

    mclg::GBMClientBufferFactory factory(drm_fd_handler);

    geom::Size unused_size{0, 0};
    auto buffer = factory.create_buffer(package, unused_size, pf);

    auto const& buf_size = buffer->size();
    EXPECT_EQ(package->width, buf_size.width.as_int());
    EXPECT_EQ(package->height, buf_size.height.as_int());

    EXPECT_NE(unused_size, buf_size);
}

