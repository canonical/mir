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

#include "mir/graphics/android/mir_native_buffer.h"

namespace mga=mir::graphics::android;

namespace
{
void incref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<mga::AndroidNativeBuffer*>(base);
    buffer->driver_reference();
}
void decref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<mga::AndroidNativeBuffer*>(base);
    buffer->driver_dereference();
}
}

mga::AndroidNativeBuffer::AndroidNativeBuffer(
    std::shared_ptr<const native_handle_t> const& handle,
    std::shared_ptr<Fence> const& fence)
    : fence(fence),
      native_buffer(std::make_shared<ANativeWindowBuffer>()),
      handle_resource(handle),
      mir_reference(true),
      driver_references(0)
{
    native_buffer->common.incRef = incref_hook;
    native_buffer->common.decRef = decref_hook;
    native_buffer->handle = handle_resource.get();
}


void mga::AndroidNativeBuffer::driver_reference()
{
    std::unique_lock<std::mutex> lk(mutex);
    driver_references++;
}

void mga::AndroidNativeBuffer::driver_dereference()
{
    std::unique_lock<std::mutex> lk(mutex);
    driver_references--;
    if ((!mir_reference) && (driver_references == 0))
    {
        lk.unlock();
        delete this;
    }
}

void mga::AndroidNativeBuffer::mir_dereference()
{
    std::unique_lock<std::mutex> lk(mutex);
    mir_reference = false;
    if (driver_references == 0)
    {
        lk.unlock();
        delete this;
    }
}

void mga::AndroidNativeBuffer::wait_for_content()
{
    fence->wait();
}


void mga::AndroidNativeBuffer::update_fence(NativeFence& merge_fd)
{
    fence->merge_with(merge_fd);
}

mga::AndroidNativeBuffer::operator ANativeWindowBuffer*()
{
    return native_buffer.get();
}

mga::AndroidNativeBuffer::operator mga::NativeFence() const
{
    return fence->copy_native_handle();
}

mga::AndroidNativeBuffer::~AndroidNativeBuffer()
{
}
