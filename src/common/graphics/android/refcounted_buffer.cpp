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

namespace
{
static void incref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<mga::RefCountedNativeBuffer*>(base);
    buffer->driver_reference();
}
void decref_hook(struct android_native_base_t* base)
{
    auto buffer = reinterpret_cast<mga::RefCountedNativeBuffer*>(base);
    buffer->driver_dereference();
}
}

void mga::RefCountedNativeBuffer::driver_reference()
{
    std::unique_lock<std::mutex> lk(mutex);
    driver_references++;
}

void mga::RefCountedNativeBuffer::driver_dereference()
{
    std::unique_lock<std::mutex> lk(mutex);
    driver_references--;
    if ((!mir_reference) && (driver_references == 0))
    {
        lk.unlock();
        delete this;
    }
}

void mga::RefCountedNativeBuffer::mir_dereference()
{
    std::unique_lock<std::mutex> lk(mutex);
    mir_reference = false;
    if (driver_references == 0)
    {
        lk.unlock();
        delete this;
    }
}

mga::RefCountedNativeBuffer::RefCountedNativeBuffer(
    std::shared_ptr<const native_handle_t> const& handle)
    : handle_resource(handle),
      mir_reference(true),
      driver_references(0)
{
    common.incRef = incref_hook;
    common.decRef = decref_hook;
}
