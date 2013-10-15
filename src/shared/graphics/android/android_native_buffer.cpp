/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_native_buffer.h"

namespace mga=mir::graphics::android;

mga::AndroidNativeBuffer::AndroidNativeBuffer(
    std::shared_ptr<ANativeWindowBuffer> const& anwb,
    std::shared_ptr<Fence> const& fence)
    : fence(fence),
      native_window_buffer(anwb)
{
}

void mga::AndroidNativeBuffer::wait_for_content()
{
    fence->wait();
}

void mga::AndroidNativeBuffer::update_fence(NativeFence& merge_fd)
{
    fence->merge_with(merge_fd);
}

ANativeWindowBuffer* mga::AndroidNativeBuffer::anwb() const
{
    return native_window_buffer.get();
}

buffer_handle_t mga::AndroidNativeBuffer::handle() const
{
    return native_window_buffer->handle;
}

mga::NativeFence mga::AndroidNativeBuffer::copy_fence() const
{
    return fence->copy_native_handle();
}
