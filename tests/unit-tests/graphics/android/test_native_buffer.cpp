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

#include "mir/graphics/android/android_native_buffer.h"
#include "mir_test_doubles/mock_fence.h"
#include <memory>
#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;

struct NativeBuffer : public testing::Test
{
    NativeBuffer() :
        a_native_window_buffer(std::make_shared<ANativeWindowBuffer>()),
        mock_fence(std::make_shared<mtd::MockFence>()),
        fake_fd{48484}
    {
    }

    std::shared_ptr<ANativeWindowBuffer> a_native_window_buffer;
    std::shared_ptr<mtd::MockFence> mock_fence;
    int fake_fd;
};

TEST_F(NativeBuffer, external_refcount_driver_hooks)
{
    auto native_handle_resource = std::make_shared<native_handle_t>();
    auto use_count_before = native_handle_resource.use_count();
    ANativeWindowBuffer* driver_reference = nullptr;
    {
        auto tmp = new mga::RefCountedNativeBuffer(native_handle_resource);
        std::shared_ptr<mga::RefCountedNativeBuffer> buffer(tmp, [](mga::RefCountedNativeBuffer* buffer)
            {
                buffer->mir_dereference();
            });

        driver_reference = buffer.get();
        driver_reference->common.incRef(&driver_reference->common);
        //Mir loses its reference, driver still has a ref
    }

    EXPECT_EQ(use_count_before+1, native_handle_resource.use_count());
    driver_reference->common.decRef(&driver_reference->common);
    EXPECT_EQ(use_count_before, native_handle_resource.use_count());
}

TEST_F(NativeBuffer, driver_hooks_mir_ref)
{
    auto native_handle_resource = std::make_shared<native_handle_t>();
    auto use_count_before = native_handle_resource.use_count();
    {
        std::shared_ptr<mga::RefCountedNativeBuffer> mir_reference;
        ANativeWindowBuffer* driver_reference = nullptr;
        {
            auto tmp = new mga::RefCountedNativeBuffer(native_handle_resource);
            mir_reference = std::shared_ptr<mga::RefCountedNativeBuffer>(tmp, [](mga::RefCountedNativeBuffer* buffer)
                {
                    buffer->mir_dereference();
                });
            driver_reference = tmp;
            driver_reference->common.incRef(&driver_reference->common);
        }

        //driver loses its reference
        driver_reference->common.decRef(&driver_reference->common);
        EXPECT_EQ(use_count_before+1, native_handle_resource.use_count());
    }
    EXPECT_EQ(use_count_before, native_handle_resource.use_count());
}

TEST_F(NativeBuffer, access_for_read_while_being_read)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(0);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::read);
    buffer.ensure_available_for(mir::graphics::Access::read);
}

TEST_F(NativeBuffer, access_for_read_while_being_written)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::write);
    buffer.ensure_available_for(mir::graphics::Access::read);
}

TEST_F(NativeBuffer, access_for_write_while_being_read)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::read);
    buffer.ensure_available_for(mir::graphics::Access::write);
}

TEST_F(NativeBuffer, access_for_write_while_being_written)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::write);
    buffer.ensure_available_for(mir::graphics::Access::write);
}

TEST_F(NativeBuffer, update_usage)
{
    EXPECT_CALL(*mock_fence, merge_with(fake_fd))
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::read);
    buffer.update_usage(fake_fd, mir::graphics::Access::write);
}

TEST_F(NativeBuffer, updates_access)
{
    EXPECT_CALL(*mock_fence, wait(fake_fd))
        .Times(3);

    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_fence, mir::graphics::Access::read);
    buffer.ensure_available_for(mir::graphics::Access::write);
    buffer.ensure_available_for(mir::graphics::Access::read);

    buffer.update_usage(fake_fd, mir::graphics::Access::write);
    buffer.ensure_available_for(mir::graphics::Access::write);
    buffer.ensure_available_for(mir::graphics::Access::read);
}
