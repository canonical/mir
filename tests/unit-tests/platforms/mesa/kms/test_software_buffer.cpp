/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/platforms/mesa/server/software_buffer.h"
#include "src/platforms/mesa/include/native_buffer.h"
#include "mir/shm_file.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mgm = mir::graphics::mesa;
namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;

namespace
{
struct StubShmFile : public mir::ShmFile
{
    void* base_ptr() const { return fake_mapping; }
    int fd() const { return fake_fd; }

    void* const fake_mapping = reinterpret_cast<void*>(0x12345678);
    int const fake_fd = 17;
};

struct SoftwareBufferTest : public testing::Test
{
    SoftwareBufferTest() :
        stub_shm_file{new StubShmFile},
        buffer{std::unique_ptr<StubShmFile>(stub_shm_file), size, pixel_format}
    {
    }

    geom::Size const size{12, 1000};
    MirPixelFormat const pixel_format { mir_pixel_format_bgr_888 };
    StubShmFile* stub_shm_file;
    mgm::SoftwareBuffer buffer;
};
}

TEST_F(SoftwareBufferTest, native_buffer_contains_correct_data)
{
    size_t const bytes_per_pixel = MIR_BYTES_PER_PIXEL(pixel_format);
    size_t const expected_stride{bytes_per_pixel * size.width.as_uint32_t()};

    auto native_buffer = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer.native_buffer_handle());
    ASSERT_THAT(native_buffer, testing::Ne(nullptr));
    EXPECT_EQ(1, native_buffer->fd_items);
    EXPECT_EQ(stub_shm_file->fake_fd, native_buffer->fd[0]);

    EXPECT_EQ(size.width.as_int(), native_buffer->width);
    EXPECT_EQ(size.height.as_int(), native_buffer->height);
    EXPECT_EQ(expected_stride, static_cast<size_t>(native_buffer->stride));
}

TEST_F(SoftwareBufferTest, cannot_be_used_for_bypass)
{
    auto native_buffer = std::dynamic_pointer_cast<mgm::NativeBuffer>(buffer.native_buffer_handle());
    ASSERT_THAT(native_buffer, testing::Ne(nullptr));
    EXPECT_FALSE(native_buffer->flags & mir_buffer_flag_can_scanout);
}
