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

#include "interpreter_cache.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;

void mga::InterpreterCache::store_buffer(std::shared_ptr<mg::Buffer>const& buffer, ANativeWindowBuffer* key)
{
    buffers_in_driver[key] = buffer;
}

std::shared_ptr<mg::Buffer> mga::InterpreterCache::retrieve_buffer(ANativeWindowBuffer* returned_handle)
{
    auto buffer_it = buffers_in_driver.find(returned_handle); 
    if (buffer_it == buffers_in_driver.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }
    auto buffer_out = buffer_it->second;
    buffers_in_driver.erase(buffer_it);
    return buffer_out;
}
