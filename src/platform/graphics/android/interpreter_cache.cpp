/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "interpreter_cache.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;

void mga::InterpreterCache::store_buffer(std::shared_ptr<mg::Buffer>const& buffer,
    std::shared_ptr<mg::NativeBuffer> const& key)
{
    native_buffers[key->anwb()] = key;
    buffers_in_driver[key->anwb()] = buffer;
}

std::shared_ptr<mg::Buffer> mga::InterpreterCache::retrieve_buffer(ANativeWindowBuffer* returned_handle)
{
    auto buffer_it = buffers_in_driver.find(returned_handle);
    auto native_it = native_buffers.find(returned_handle);

    if ((buffer_it == buffers_in_driver.end()) ||
        (native_it == native_buffers.end()))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }
    native_buffers.erase(native_it);

    auto buffer_out = buffer_it->second;
    buffers_in_driver.erase(buffer_it);
    return buffer_out;
}

void mga::InterpreterCache::update_native_fence(ANativeWindowBuffer* key, int fence)
{
    auto native_it = native_buffers.find(key);
    if (native_it == native_buffers.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }

    auto native_buffer = native_it->second;
    native_buffer->update_fence(fence);
}
