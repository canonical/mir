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
#include "mir/graphics/egl_sync_fence.h"
#include "mir/test/doubles/mock_fence.h"
#include <memory>
#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
using namespace testing;

namespace
{
struct MockCommandStreamSync : public mir::graphics::CommandStreamSync
{
    MOCK_METHOD0(raise, void());
    MOCK_METHOD0(reset, void());
    MOCK_METHOD1(wait_for, bool(std::chrono::nanoseconds));
};
}

struct NativeBuffer : public testing::Test
{
    NativeBuffer() :
        a_native_window_buffer(std::make_shared<ANativeWindowBuffer>()),
        mock_fence(std::make_shared<testing::NiceMock<mtd::MockFence>>()),
        fake_fd{48484},
        timeout{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(2))},
        mock_cmdstream_sync{std::make_shared<MockCommandStreamSync>()}
    {
    }

    std::shared_ptr<ANativeWindowBuffer> a_native_window_buffer;
    std::shared_ptr<mtd::MockFence> mock_fence;
    int fake_fd;
    std::chrono::nanoseconds timeout;
    std::shared_ptr<MockCommandStreamSync> mock_cmdstream_sync;
};

TEST_F(NativeBuffer, extends_lifetime_when_driver_calls_external_refcount_hooks)
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

TEST_F(NativeBuffer, is_not_destroyed_if_driver_loses_reference_while_mir_still_has_reference)
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

TEST_F(NativeBuffer, does_not_wait_for_read_access_while_being_read)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(0);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    buffer.ensure_available_for(mga::BufferAccess::read);
}

TEST_F(NativeBuffer, waits_for_read_access_while_being_written)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::write);
    buffer.ensure_available_for(mga::BufferAccess::read);
}

TEST_F(NativeBuffer, waits_for_write_access_while_being_read)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    buffer.ensure_available_for(mga::BufferAccess::write);
}

TEST_F(NativeBuffer, waits_for_write_access_while_being_written)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::write);
    buffer.ensure_available_for(mga::BufferAccess::write);
}

TEST_F(NativeBuffer, merges_existing_fence_with_updated_fence)
{
    EXPECT_CALL(*mock_fence, merge_with(fake_fd))
        .Times(1);
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    buffer.update_usage(fake_fd, mga::BufferAccess::write);
}

TEST_F(NativeBuffer, waits_depending_on_last_fence_update)
{
    EXPECT_CALL(*mock_fence, wait())
        .Times(3);

    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    buffer.ensure_available_for(mga::BufferAccess::write);
    buffer.ensure_available_for(mga::BufferAccess::read);

    buffer.update_usage(fake_fd, mga::BufferAccess::write);
    buffer.ensure_available_for(mga::BufferAccess::write);
    buffer.ensure_available_for(mga::BufferAccess::read);
}

TEST_F(NativeBuffer, raises_egl_fence)
{
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);

    EXPECT_CALL(*mock_cmdstream_sync, raise());
    buffer.lock_for_gpu();
}

TEST_F(NativeBuffer, clears_egl_fence_successfully)
{
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    EXPECT_CALL(*mock_cmdstream_sync, wait_for(timeout))
        .Times(1)
        .WillOnce(Return(true));
    buffer.wait_for_unlock_by_gpu();
}

//not really helpful to throw if the timeout fails
TEST_F(NativeBuffer, ignores_clears_egl_fence_failure)
{
    mga::AndroidNativeBuffer buffer(a_native_window_buffer, mock_cmdstream_sync, mock_fence, mga::BufferAccess::read);
    EXPECT_CALL(*mock_cmdstream_sync, wait_for(timeout))
        .Times(1)
        .WillOnce(Return(false));
    buffer.wait_for_unlock_by_gpu();
}
