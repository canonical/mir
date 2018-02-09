/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "src/platforms/mesa/client/client_buffer.h"
#include "src/platforms/mesa/client/client_buffer_factory.h"
#include "src/platforms/mesa/client/buffer_file_ops.h"
#include "src/platforms/mesa/include/native_buffer.h"
#include "mir/test/doubles/mock_egl.h"

#include <sys/mman.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace geom=mir::geometry;
namespace mclg=mir::client::mesa;
namespace mgm=mir::graphics::mesa;
using namespace testing;
namespace
{

class MockBufferFileOps : public mclg::BufferFileOps
{
public:
    MOCK_CONST_METHOD1(close, int(int fd));
    MOCK_CONST_METHOD3(map, void*(int fd, off_t offset, size_t size));
    MOCK_CONST_METHOD2(unmap, void(void* addr, size_t size));
};

struct MesaClientBufferTest : public Test
{
    void SetUp()
    {
        width = geom::Width(12);
        height =geom::Height(14);
        stride = geom::Stride(66);
        pf = mir_pixel_format_abgr_8888;
        size = geom::Size{width, height};
        buffer_file_ops = std::make_shared<NiceMock<MockBufferFileOps>>();

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

    std::shared_ptr<NiceMock<MockBufferFileOps>> buffer_file_ops;
    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MirBufferPackage> package_copy;

};

}

TEST_F(MesaClientBufferTest, width_and_height)
{
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    EXPECT_EQ(buffer.size().height, height);
    EXPECT_EQ(buffer.size().width, width);
    EXPECT_EQ(buffer.pixel_format(), pf);
}

TEST_F(MesaClientBufferTest, buffer_returns_correct_stride)
{
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    EXPECT_EQ(buffer.stride(), stride);
}

TEST_F(MesaClientBufferTest, buffer_returns_set_package)
{
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);

    auto package_return = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer.native_buffer_handle());
    ASSERT_THAT(package_return, Ne(nullptr));
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
    EXPECT_CALL(*buffer_file_ops, close(package->fd[0]))
        .Times(1);

    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);
}

TEST_F(MesaClientBufferTest, factory_gets_size_from_package)
{
    mclg::ClientBufferFactory factory(buffer_file_ops);

    geom::Size unused_size{0, 0};
    auto buffer = factory.create_buffer(package, unused_size, pf);

    auto const& buf_size = buffer->size();
    EXPECT_EQ(package->width, buf_size.width.as_int());
    EXPECT_EQ(package->height, buf_size.height.as_int());

    EXPECT_NE(unused_size, buf_size);
}

TEST_F(MesaClientBufferTest, creation_with_invalid_buffer_package_throws)
{
    mclg::ClientBufferFactory factory(buffer_file_ops);
    auto const invalid_package = std::make_shared<MirBufferPackage>();
    package->fd_items = 0;

    geom::Size const unused_size{0, 0};
    EXPECT_THROW({
        factory.create_buffer(invalid_package, unused_size, pf);
    }, std::runtime_error);
}

TEST_F(MesaClientBufferTest, packs_empty_update_msg)
{
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);
    MirBufferPackage msg;
    msg.data_items = 2;
    msg.fd_items = 2;

    buffer.fill_update_msg(msg);
    EXPECT_THAT(msg.data_items, Eq(0)); 
    EXPECT_THAT(msg.fd_items, Eq(0)); 
}

TEST_F(MesaClientBufferTest, suggests_dma_import)
{
    static EGLint expected_image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_WIDTH, static_cast<EGLint>(package->width),
        EGL_HEIGHT, static_cast<EGLint>(package->height),
        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(GBM_FORMAT_ABGR8888),
        EGL_DMA_BUF_PLANE0_FD_EXT, package->fd[0],
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(package->stride),
        EGL_NONE
    };

    EGLenum type;
    EGLClientBuffer egl_buffer;
    EGLint* attrs;
    
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, pf);
    buffer.egl_image_creation_parameters(&type, &egl_buffer, &attrs);

    EXPECT_THAT(type, Eq(static_cast<EGLenum>(EGL_LINUX_DMA_BUF_EXT)));
    EXPECT_THAT(egl_buffer, Eq(nullptr));
    EXPECT_THAT(attrs, mir::test::doubles::AttrMatches(expected_image_attrs));
}

//LP: #1661521
TEST_F(MesaClientBufferTest, suggests_dma_import_with_native_format)
{
    auto format = GBM_FORMAT_ABGR8888;
    static EGLint expected_image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_WIDTH, static_cast<EGLint>(package->width),
        EGL_HEIGHT, static_cast<EGLint>(package->height),
        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(format),
        EGL_DMA_BUF_PLANE0_FD_EXT, package->fd[0],
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(package->stride),
        EGL_NONE
    };

    EGLenum type;
    EGLClientBuffer egl_buffer;
    EGLint* attrs;
    
    mclg::ClientBuffer buffer(buffer_file_ops, package, size, format, 0);
    buffer.egl_image_creation_parameters(&type, &egl_buffer, &attrs);

    EXPECT_THAT(type, Eq(static_cast<EGLenum>(EGL_LINUX_DMA_BUF_EXT)));
    EXPECT_THAT(egl_buffer, Eq(nullptr));
    EXPECT_THAT(attrs, mir::test::doubles::AttrMatches(expected_image_attrs));
}

TEST_F(MesaClientBufferTest, invalid_pixel_format_throws)
{
    EXPECT_THROW({ 
        mclg::ClientBuffer buffer(buffer_file_ops, package, size, mir_pixel_format_invalid);
    }, std::invalid_argument);
}
