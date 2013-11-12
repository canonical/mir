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

#include "mir_test_doubles/mock_android_registrar.h"
#include "src/client/android/android_client_buffer.h"
#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <algorithm>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mcl = mir::client;
namespace mcla = mir::client::android;
namespace geom = mir::geometry;

class ClientAndroidBufferTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        height = geom::Height(124);
        width = geom::Width(248);
        size = geom::Size{width, height};
        stride = geom::Stride{66};
        pf = geom::PixelFormat::abgr_8888;

        mock_android_registrar = std::make_shared<NiceMock<mtd::MockAndroidRegistrar>>();

        package = std::make_shared<native_handle_t>();
    }

    std::shared_ptr<native_handle_t> package;
    geom::Size size;
    geom::Height height;
    geom::Width width;
    geom::Stride stride;
    geom::PixelFormat pf;
    std::shared_ptr<mcla::AndroidClientBuffer> buffer;
    std::shared_ptr<mtd::MockAndroidRegistrar> mock_android_registrar;
};

TEST_F(ClientAndroidBufferTest, buffer_returns_width_without_modifying)
{
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    EXPECT_EQ(size, buffer.size());
}

TEST_F(ClientAndroidBufferTest, buffer_returns_pf_without_modifying)
{
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    EXPECT_EQ(pf, buffer.pixel_format());
}

TEST_F(ClientAndroidBufferTest, buffer_returns_correct_stride)
{
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    EXPECT_EQ(stride, buffer.stride());
}

TEST_F(ClientAndroidBufferTest, buffer_uses_registrar_for_secure)
{
    using namespace testing;
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);

    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));

    buffer.secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, buffer_uses_right_handle_to_secure)
{
    using namespace testing;
    geom::Point point{0, 0};
    geom::Size size{width, height};
    geom::Rectangle rect{point, size};
    std::shared_ptr<const native_handle_t> tmp = package;
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(tmp,rect))
        .Times(1)
        .WillOnce(Return(std::shared_ptr<char>()));

    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    auto region = buffer.secure_for_cpu_write();
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_right_vaddr)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));

    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    auto region = buffer.secure_for_cpu_write();
    EXPECT_EQ(empty_char, region->vaddr);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_memory_region_with_correct_buffer_dimensions)
{
    using namespace testing;
    EXPECT_CALL(*mock_android_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(std::shared_ptr<char>()));

    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    auto region = buffer.secure_for_cpu_write();

    EXPECT_EQ(width, region->width);
    EXPECT_EQ(height, region->height);
    EXPECT_EQ(stride, region->stride);
    EXPECT_EQ(pf, region->format);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_info)
{
    int correct_usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    int32_t const expected_stride_in_pixels =
        static_cast<int32_t>(stride.as_uint32_t() / geom::bytes_per_pixel(pf));
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    auto native_handle = buffer.native_buffer_handle();
    ASSERT_NE(nullptr, native_handle);

    auto anwb = native_handle->anwb();
    ASSERT_NE(nullptr, anwb);

    EXPECT_EQ(package.get(), anwb->handle);
    EXPECT_EQ(width.as_uint32_t(), static_cast<uint32_t>(anwb->width));
    EXPECT_EQ(height.as_uint32_t(), static_cast<uint32_t>(anwb->height));
    EXPECT_EQ(correct_usage, anwb->usage);
    EXPECT_EQ(expected_stride_in_pixels, anwb->stride);
}

TEST_F(ClientAndroidBufferTest, buffer_packs_anativewindowbuffer_refcounters_set)
{
    mcla::AndroidClientBuffer buffer(mock_android_registrar, package, size, pf, stride);
    auto native_handle = buffer.native_buffer_handle();
    auto anwb = native_handle->anwb();

    ASSERT_NE(nullptr, anwb);
    ASSERT_NE(nullptr, anwb->common.incRef);
    ASSERT_NE(nullptr, anwb->common.decRef);

    anwb->common.incRef(&anwb->common);
    anwb->common.decRef(&anwb->common);
}
