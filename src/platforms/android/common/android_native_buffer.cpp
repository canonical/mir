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

#include "command_stream_sync.h"
#include "android_native_buffer.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

mga::AndroidNativeBuffer::AndroidNativeBuffer(
    std::shared_ptr<ANativeWindowBuffer> const& anwb,
    std::shared_ptr<CommandStreamSync> const& cmdstream_sync,
    std::shared_ptr<Fence> const& fence,
    BufferAccess access) :
    cmdstream_sync(cmdstream_sync),
    fence_(fence),
    access(access),
    native_window_buffer(anwb)
{
}

void mga::AndroidNativeBuffer::ensure_available_for(BufferAccess intent)
{
    if ((access == mga::BufferAccess::read) && (intent == mga::BufferAccess::read))
        return;
  
    fence_->wait();
}

bool mga::AndroidNativeBuffer::ensure_available_for(BufferAccess intent, std::chrono::milliseconds ms)
{
    if ((access == mga::BufferAccess::read) && (intent == mga::BufferAccess::read))
        return true;

    return fence_->wait_for(ms);
}

void mga::AndroidNativeBuffer::update_usage(NativeFence& merge_fd, BufferAccess updated_access)
{
    fence_->merge_with(merge_fd);
    access = updated_access;
}

void mga::AndroidNativeBuffer::reset_fence()
{
    fence_->reset_fence();
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
    return fence_->copy_native_handle();
}

mga::NativeFence mga::AndroidNativeBuffer::fence() const
{
    return fence_->native_handle();
}

void mga::AndroidNativeBuffer::lock_for_gpu()
{
    cmdstream_sync->raise();
}

void mga::AndroidNativeBuffer::wait_for_unlock_by_gpu()
{
    using namespace std::chrono;
    cmdstream_sync->wait_for(duration_cast<nanoseconds>(seconds(2)));
}

mga::NativeBuffer* mga::to_native_buffer_checked(mg::NativeBuffer* buffer)
{
    if (auto native = dynamic_cast<mga::NativeBuffer*>(buffer))
        return native;
    BOOST_THROW_EXCEPTION(std::invalid_argument("cannot downcast mg::NativeBuffer to android::NativeBuffer"));
}

std::shared_ptr<mga::NativeBuffer> mga::to_native_buffer_checked(std::shared_ptr<mg::NativeBuffer> const& buffer)
{
    if (auto native = std::dynamic_pointer_cast<mga::NativeBuffer>(buffer))
        return native;
    BOOST_THROW_EXCEPTION(std::invalid_argument("cannot downcast mg::NativeBuffer to android::NativeBuffer"));
}
