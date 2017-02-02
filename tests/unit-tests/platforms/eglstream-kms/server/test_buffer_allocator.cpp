/*
 * Copyright © 2016 Canonical Ltd.
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

#include "mir/graphics/buffer_properties.h"
#include "src/platforms/eglstream-kms/server/buffer_allocator.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace mge = mir::graphics::eglstream;
namespace mg = mir::graphics;

struct BufferAllocator : testing::Test
{
};

TEST_F(BufferAllocator, cannot_alloc_hardware_buffer)
{
    mge::BufferAllocator allocator;
    EXPECT_THROW({
        allocator.alloc_buffer({1, 1}, 0u, 0u);
    }, std::runtime_error);
}

TEST_F(BufferAllocator, cannot_alloc_sw_buffer_from_list)
{
    mge::BufferAllocator allocator;
    auto supported = allocator.supported_pixel_formats();
    ASSERT_THAT(supported, Not(IsEmpty()));
    auto buffer = allocator.alloc_software_buffer({1, 1}, supported.front());
    EXPECT_THAT(buffer, Not(IsNull()));
}

TEST_F(BufferAllocator, cannot_alloc_hardware_buffer_legacy)
{
    mge::BufferAllocator allocator;
    EXPECT_THROW({
        allocator.alloc_buffer(
            mg::BufferProperties{ { 1, 1}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware});
    }, std::runtime_error);
}

TEST_F(BufferAllocator, can_alloc_hardware_buffer_legacy)
{
    mge::BufferAllocator allocator;
    mir::geometry::Size size { 1, 22 };
    auto supported = allocator.supported_pixel_formats();
    ASSERT_THAT(supported, Not(IsEmpty()));
    auto buffer = allocator.alloc_buffer(
            mg::BufferProperties{ size, mir_pixel_format_abgr_8888, mg::BufferUsage::software});
    ASSERT_THAT(buffer, Not(IsNull()));
    EXPECT_THAT(buffer->size(), Eq(size));
}
