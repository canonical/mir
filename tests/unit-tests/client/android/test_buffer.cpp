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

#include "mir_test_doubles/mock_buffer_registrar.h"
#include "src/client/android/buffer.h"
#include "mir_toolkit/mir_client_library.h"

#include <memory>
#include <algorithm>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mcla = mir::client::android;
namespace geom = mir::geometry;

struct AndroidClientBuffer : public ::testing::Test
{
    AndroidClientBuffer() :
    mock_registrar{std::make_shared<testing::NiceMock<mtd::MockBufferRegistrar>>()},
    package{std::make_shared<native_handle_t>()}
    {
    }

    geom::Height const height{124};
    geom::Width const width{248};
    geom::Size const size{width, height};
    geom::Stride const stride{66};
    MirPixelFormat const pf{mir_pixel_format_abgr_8888};
    std::shared_ptr<mtd::MockBufferRegistrar> const mock_registrar;
    std::shared_ptr<native_handle_t const> const package;
};

TEST_F(AndroidClientBuffer, returns_properties_from_constructor)
{
    mcla::Buffer buffer(mock_registrar, package, size, pf, stride);
    EXPECT_EQ(size, buffer.size());
    EXPECT_EQ(pf, buffer.pixel_format());
    EXPECT_EQ(stride, buffer.stride());
}

TEST_F(AndroidClientBuffer, secures_for_cpu_with_correct_rect_and_handle)
{
    geom::Point point{0, 0};
    geom::Rectangle rect{point, size};

    EXPECT_CALL(*mock_registrar, secure_for_cpu(package,rect))
        .Times(1)
        .WillOnce(testing::Return(std::make_shared<char>()));

    mcla::Buffer buffer(mock_registrar, package, size, pf, stride);
    buffer.secure_for_cpu_write();
}

TEST_F(AndroidClientBuffer, packs_memory_region_correctly)
{
    using namespace testing;
    std::shared_ptr<char> empty_char = std::make_shared<char>();
    EXPECT_CALL(*mock_registrar, secure_for_cpu(_,_))
        .Times(1)
        .WillOnce(Return(empty_char));

    mcla::Buffer buffer(mock_registrar, package, size, pf, stride);
    auto region = buffer.secure_for_cpu_write();
    EXPECT_EQ(empty_char, region->vaddr);
    EXPECT_EQ(width, region->width);
    EXPECT_EQ(height, region->height);
    EXPECT_EQ(stride, region->stride);
    EXPECT_EQ(pf, region->format);
}

TEST_F(AndroidClientBuffer, produces_valid_anwb)
{
    using namespace testing;
    int correct_usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    int32_t const expected_stride_in_pixels =
        static_cast<int32_t>(stride.as_uint32_t() / MIR_BYTES_PER_PIXEL(pf));
    mcla::Buffer buffer(mock_registrar, package, size, pf, stride);
    auto native_handle = buffer.native_buffer_handle();

    ASSERT_THAT(native_handle, Ne(nullptr));
    auto anwb = native_handle->anwb();
    ASSERT_THAT(anwb, Ne(nullptr));
    EXPECT_EQ(package.get(), anwb->handle);
    EXPECT_EQ(width.as_uint32_t(), static_cast<uint32_t>(anwb->width));
    EXPECT_EQ(height.as_uint32_t(), static_cast<uint32_t>(anwb->height));
    EXPECT_EQ(correct_usage, anwb->usage);
    EXPECT_EQ(expected_stride_in_pixels, anwb->stride);
    ASSERT_THAT(anwb->common.incRef, Ne(nullptr));
    ASSERT_THAT(anwb->common.decRef, Ne(nullptr));
    anwb->common.incRef(&anwb->common);
    anwb->common.decRef(&anwb->common);
}
